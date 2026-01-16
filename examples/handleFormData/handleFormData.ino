#include <FS.h>
#include <LittleFS.h>
#include <FSWebServer.h>   // https://github.com/cotestatnt/esp-fs-webserver/

#define FILESYSTEM LittleFS
FSWebServer server(LittleFS, 80, "esphost");

////////////////////////////  HTTP Request Handlers  ////////////////////////////////////
void getDefaultValue () {
  // Send to client default values as JSON string because it's very easy to parse JSON in Javascript
  String defaultVal = "{\"car\":\"Ferrari\", \"firstname\":\"Enzo\", \"lastname\":\"Ferrari\",\"age\":90}";
  server.send(200, "text/json", defaultVal);
}

void handleForm1() {
  String reply;
  if(server.hasArg("cars")) {
    reply += "You have submitted with Form1: ";
    reply += server.arg("cars");
  }
  Serial.println(reply);
  server.send(200, "text/plain", reply);
}

void handleForm2() {
  String reply;
  if(server.hasArg("firstname")) {
    reply += "You have submitted with Form2: ";
    reply += server.arg("firstname");
  }
  if(server.hasArg("lastname")) {
    reply += " ";
    reply += server.arg("lastname");
  }
  if(server.hasArg("age")) {
    reply += ", age: ";
    reply += server.arg("age");
  }
  Serial.println(reply);
  server.send(200, "text/plain", reply);
}

////////////////////////////////  Filesystem  /////////////////////////////////////////
bool startFilesystem() {
  if (FILESYSTEM.begin()){
    server.printFileList(LittleFS, "/", 1, Serial);   
    return true;
  }
  else {
      Serial.println("ERROR on mounting filesystem. It will be reformatted!");
      FILESYSTEM.format();
      ESP.restart();
  }
  return false;
}


void setup(){
  Serial.begin(115200);
  
  // FILESYSTEM INIT
  startFilesystem();

  // Try to connect to WiFi (will start AP if not connected after timeout)
  if (!server.startWiFi(10000)) {
    Serial.println("\nWiFi not connected! Starting AP mode...");
    server.startCaptivePortal("ESP_AP", "123456789", "/setup");
  }

  // Add custom page handlers to webserver
  server.on("/getDefault", HTTP_GET, getDefaultValue);
  server.on("/setForm1", HTTP_POST, handleForm1);
  server.on("/setForm2", HTTP_POST, handleForm2);

  // Enable ACE FS file web editor and add FS info callback function
  server.enableFsCodeEditor();

  // Start server
  server.begin();
  Serial.print(F("ESP Web Server started on IP Address: "));
  Serial.println(server.getServerIP());
  Serial.println(F(
    "This is \"handleFormData.ino\" example.\n"
    "Open /setup page to configure optional parameters.\n"
    "Open /edit page to view, edit or upload example or your custom webserver source files."
  ));
}


void loop() {
  // Handle client requests
  server.run();
  
  // Nothing to do here, just a small delay
  delay(10);  
}
