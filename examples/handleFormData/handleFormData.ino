#include <esp-fs-webserver.h>   // https://github.com/cotestatnt/esp-fs-webserver

#include <FS.h>
#include <LittleFS.h>
#define FILESYSTEM LittleFS

FSWebServer myWebServer(FILESYSTEM, 80);

////////////////////////////  HTTP Request Handlers  ////////////////////////////////////
void getDefaultValue() {  
  // Send to client default values as JSON string because it's very easy to parse JSON in Javascript
  String defaultVal = "{\"car\":\"Ferrari\", \"firstname\":\"Enzo\", \"lastname\":\"Ferrari\",\"age\":90}";
  myWebServer.send(200, "text/json", defaultVal);
}

void handleForm1() {  
  String reply;
  if(myWebServer.hasArg("cars")) {
    reply += "You have submitted with Form1: ";
    reply += myWebServer.arg("cars");
  }
  Serial.println(reply);
  myWebServer.send(200, "text/plain", reply);
}

void handleForm2() {  
  String reply;
  if(myWebServer.hasArg("firstname")) {
    reply += "You have submitted with Form2: ";
    reply += myWebServer.arg("firstname");
  }
  if(myWebServer.hasArg("lastname")) {
    reply += " ";
    reply += myWebServer.arg("lastname");
  }
  if(myWebServer.hasArg("age")) {
    reply += ", age: ";
    reply += myWebServer.arg("age");
  }
  Serial.println(reply);
  myWebServer.send(200, "text/plain", reply);
}


////////////////////////////////  Filesystem  /////////////////////////////////////////
void startFilesystem() {
  // FILESYSTEM INIT
  if ( !FILESYSTEM.begin()) {
    Serial.println("ERROR on mounting filesystem. It will be formmatted!");
    FILESYSTEM.format();
    ESP.restart();
  }
  myWebServer.printFileList(LittleFS, Serial, "/", 2);
}

/*
* Getting FS info (total and free bytes) is strictly related to
* filesystem library used (LittleFS, FFat, SPIFFS etc etc) and ESP framework
* ESP8266 FS implementation has methods for total and used bytes (only label is missing)
*/
#ifdef ESP32
void getFsInfo(fsInfo_t* fsInfo) {
	fsInfo->fsName = "LittleFS";
	fsInfo->totalBytes = LittleFS.totalBytes();
	fsInfo->usedBytes = LittleFS.usedBytes();
}
#else
void getFsInfo(fsInfo_t* fsInfo) {
	fsInfo->fsName = "LittleFS";
}
#endif


void setup(){
  Serial.begin(115200);

  // FILESYSTEM INIT
  startFilesystem();

  // Try to connect to stored SSID, start AP if fails after timeout
  myWebServer.setAP("ESP_AP", "123456789");
  IPAddress myIP = myWebServer.startWiFi(15000);

  // Add custom page handlers to webserver
  myWebServer.on("/getDefault", HTTP_GET, getDefaultValue);
  myWebServer.on("/setForm1", HTTP_POST, handleForm1);
  myWebServer.on("/setForm2", HTTP_POST, handleForm2);
  
  // set /setup and /edit page authentication
  // myWebServer.setAuthentication("admin", "admin");

  // Enable ACE FS file web editor and add FS info callback function
  myWebServer.enableFsCodeEditor(getFsInfo);
  
  myWebServer.begin();
  Serial.print(F("ESP Web Server started on IP Address: "));
  Serial.println(myIP);
  Serial.println(F("Open /setup page to configure optional parameters"));
  Serial.println(F("Open /edit page to view and edit files"));
  Serial.println(F("Open /update page to upload firmware and filesystem updates"));
    
}


void loop() {
  myWebServer.run();
}
