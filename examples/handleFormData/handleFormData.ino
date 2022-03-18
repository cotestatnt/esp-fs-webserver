#include <esp-fs-webserver.h>   // https://github.com/cotestatnt/esp-fs-webserver

#include <FS.h>
#include <LittleFS.h>
#define FILESYSTEM LittleFS

#ifdef ESP8266
  ESP8266WebServer server(80);
#elif defined(ESP32)
  WebServer server(80);
#endif

FSWebServer myWebServer(FILESYSTEM, server);

////////////////////////////  HTTP Request Handlers  ////////////////////////////////////
void getDefaultValue() {
  WebServerClass* webRequest = myWebServer.getRequest();
  // Send to client default values as JSON string because it's very easy to parse JSON in Javascript
  String defaultVal = "{\"car\":\"Ferrari\", \"firstname\":\"Enzo\", \"lastname\":\"Ferrari\",\"age\":90}";
  webRequest->send(200, "text/json", defaultVal);
}

void handleForm1() {
  WebServerClass* webRequest = myWebServer.getRequest();
  String reply;
  if(webRequest->hasArg("cars")) {
    reply += "You have submitted with Form1: ";
    reply += webRequest->arg("cars");
  }
  Serial.println(reply);
  webRequest->send(200, "text/plain", reply);
}

void handleForm2() {
  WebServerClass* webRequest = myWebServer.getRequest();
  String reply;
  if(webRequest->hasArg("firstname")) {
    reply += "You have submitted with Form2: ";
    reply += webRequest->arg("firstname");
  }
  if(webRequest->hasArg("lastname")) {
    reply += " ";
    reply += webRequest->arg("lastname");
  }
  if(webRequest->hasArg("age")) {
    reply += ", age: ";
    reply += webRequest->arg("age");
  }
  Serial.println(reply);
  webRequest->send(200, "text/plain", reply);
}


////////////////////////////////  Filesystem  /////////////////////////////////////////
void startFilesystem(){
  // FILESYSTEM INIT
  if ( FILESYSTEM.begin()){
    File root = FILESYSTEM.open("/", "r");
    File file = root.openNextFile();
    while (file){
      const char* fileName = file.name();
      size_t fileSize = file.size();
      Serial.printf("FS File: %s, size: %lu\n", fileName, (long unsigned)fileSize);
      file = root.openNextFile();
    }
    Serial.println();
  }
  else {
    Serial.println("ERROR on mounting filesystem. It will be formatted!");
    FILESYSTEM.format();
    ESP.restart();
  }
}


void setup(){
  Serial.begin(115200);

  // FILESYSTEM INIT
  startFilesystem();

  // Try to connect to flash stored SSID, start AP if fails after timeout
  IPAddress myIP = myWebServer.startWiFi(15000, "ESP8266_AP", "123456789" );

  // Add custom page handlers to webserver
  myWebServer.addHandler("/getDefault", HTTP_GET, getDefaultValue);
  
  myWebServer.addHandler("/setForm1", HTTP_POST, handleForm1);
  myWebServer.addHandler("/setForm2", HTTP_POST, handleForm2);

  
  if (myWebServer.begin()) {
    Serial.print(F("ESP Web Server started on IP Address: "));
    Serial.println(myIP);
    Serial.println(F("Open /setup page to configure optional parameters"));
    Serial.println(F("Open /edit page to view and edit files"));
  }  
}


void loop() {
  myWebServer.run();
}
