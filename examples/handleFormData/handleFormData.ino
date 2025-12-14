#include <FS.h>
#include <LittleFS.h>
#include <FSWebServer.h>   // https://github.com/cotestatnt/esp-fs-webserver/

#define FILESYSTEM LittleFS
FSWebServer server(80, LittleFS, "esphost");

////////////////////////////  HTTP Request Handlers  ////////////////////////////////////
void getDefaultValue (AsyncWebServerRequest *request) {
  // Send to client default values as JSON string because it's very easy to parse JSON in Javascript
  String defaultVal = "{\"car\":\"Ferrari\", \"firstname\":\"Enzo\", \"lastname\":\"Ferrari\",\"age\":90}";
  request->send(200, "text/json", defaultVal);
}

void handleForm1(AsyncWebServerRequest *request) {
  String reply;
  if(request->hasArg("cars")) {
    reply += "You have submitted with Form1: ";
    reply += request->arg("cars");
  }
  Serial.println(reply);
  request->send(200, "text/plain", reply);
}

void handleForm2(AsyncWebServerRequest *request) {
  String reply;
  if(request->hasArg("firstname")) {
    reply += "You have submitted with Form2: ";
    reply += request->arg("firstname");
  }
  if(request->hasArg("lastname")) {
    reply += " ";
    reply += request->arg("lastname");
  }
  if(request->hasArg("age")) {
    reply += ", age: ";
    reply += request->arg("age");
  }
  Serial.println(reply);
  request->send(200, "text/plain", reply);
}

////////////////////////////////  Filesystem  /////////////////////////////////////////
void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  Serial.printf("\nListing directory: %s\n", dirname);
  File root = fs.open(dirname, "r");
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      if (levels) {
        #ifdef ESP32
          listDir(fs, file.path(), levels - 1);
        #elif defined(ESP8266)
          listDir(fs, file.fullName(), levels - 1);
        #endif
      }
    } else {
      Serial.printf("|__ FILE: %s (%d bytes)\n",file.name(), file.size());
    }
    file = root.openNextFile();
  }
}

bool startFilesystem() {
  if (FILESYSTEM.begin()){
    listDir(FILESYSTEM, "/", 1);
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
  /*
  * Getting FS info (total and free bytes) is strictly related to
  * filesystem library used (LittleFS, FFat, SPIFFS etc etc) and ESP framework
  * (On ESP8266 will be used "built-in" fsInfo data type)
  */
  #ifdef ESP32
  server.setFsInfoCallback( [](fsInfo_t* fsInfo) {
    fsInfo->fsName = "LittleFS";
    fsInfo->totalBytes = LittleFS.totalBytes();
    fsInfo->usedBytes = LittleFS.usedBytes();  
  });
  #endif

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
  server.handleClient();
  if (server.isAccessPointMode())
    server.updateDNS();
  
  // This delay is required in order to avoid loopTask() WDT reset on ESP32
  delay(1);  
}
