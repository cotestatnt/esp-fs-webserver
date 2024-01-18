#include <esp-fs-webserver.h>  // https://github.com/cotestatnt/esp-fs-webserver

#ifdef ESP8266
  #include <ESP8266HTTPClient.h>
  #include <ESP8266httpUpdate.h>
#elif defined(ESP32)
  #include <WiFiClientSecure.h>
  #include <HTTPClient.h>
  #include <HTTPUpdate.h>
#endif
#include <EEPROM.h>            // For storing the firmware version

#include <FS.h>
#include <LittleFS.h>
#define FILESYSTEM LittleFS

FSWebServer myWebServer(FILESYSTEM, 80);

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

uint8_t ledPin = LED_BUILTIN;
bool apMode = false;

//String fimwareInfo = "https://raw.githubusercontent.com/cotestatnt/esp-fs-webserver/main/examples/remoteOTA/version-esp32.json";
String fimwareInfo = "https://raw.githubusercontent.com/cotestatnt/esp-fs-webserver/main/examples/remoteOTA/version-esp8266.json";
char fw_version[10] = {"0.0.0"};


//////////////////////////////  Firmware update /////////////////////////////////////////
void doUpdate(const char* url, const char* version) {

  #ifdef ESP8266
  #define UPDATER ESPhttpUpdate
  #elif defined(ESP32)
  #define UPDATER httpUpdate
  #endif

  // onProgress handling is missing with ESP32 library
  UPDATER.onProgress([](int cur, int total){
      static uint32_t sendT;
      if(millis() - sendT > 1000){
          sendT = millis();
          Serial.printf("Updating %d of %d bytes...\n", cur, total);
      }
  });

  WiFiClientSecure client;
  client.setInsecure();
  UPDATER.rebootOnUpdate(false);
  UPDATER.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  UPDATER.setLedPin(LED_BUILTIN, LOW);
  t_httpUpdate_return ret = UPDATER.update(client, url, fw_version);
  client.stop();

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", UPDATER.getLastError(), UPDATER.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      strcpy(fw_version, version);
      EEPROM.put(0, fw_version);
      EEPROM.commit();
      Serial.print("System will be restarted with the new version ");
      Serial.println(fw_version);
      delay(1000);
      ESP.restart();
      break;
  }

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


////////////////////////////  HTTP Request Handlers  ////////////////////////////////////
void handleLed() {
  // http://xxx.xxx.xxx.xxx/led?val=1
  if(myWebServer.hasArg("val")) {
    int value = myWebServer.arg("val").toInt();
    digitalWrite(ledPin, value);
  }

  String reply = "LED is now ";
  reply += digitalRead(ledPin) ? "OFF" : "ON";
  myWebServer.send(200, "text/plain", reply);
}

/* Handle the update request from client.
* The web page will check if is it necessary or not checking the actual version.
* Info about firmware as version and remote url, are stored in "version.json" file
*
* Using this example, the correct workflow for deploying a new firmware version is:
  - upload the new firmware.bin compiled on your web space (in this example Github is used)
  - update the "version.json" file with the new version number and the address of the binary file
  - on the update webpage, press the "UPDATE" button.
*/
void handleUpdate() {
 
  if(myWebServer.hasArg("version") && myWebServer.hasArg("url")) {    
    const char* new_version = myWebServer.arg("version").c_str();
    const char* url = myWebServer.arg("url").c_str();
    String reply = "Firmware is going to be updated to version ";
    reply += new_version;
    reply += " from remote address ";
    reply += url;
    reply += "<br>Wait 10-20 seconds and then reload page.";
    myWebServer.send(200, "text/plain", reply );
    Serial.println(reply);
    doUpdate(url, new_version);
  }
}

///////////////////////////////////  SETUP  ///////////////////////////////////////
void setup(){
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  EEPROM.begin(128);

  // FILESYSTEM INIT
  startFilesystem();

  // Try to connect to stored SSID, start AP if fails after timeout
  myWebServer.setAP("ESP_AP", "123456789");
  IPAddress myIP = myWebServer.startWiFi(15000);

  // Configure /setup page and start Web Server
  myWebServer.addOption("New firmware JSON", fimwareInfo);

  // Add custom handlers to webserver
  myWebServer.on("/led", HTTP_GET, handleLed);
  myWebServer.on("/firmware_update", HTTP_GET, handleUpdate);

  // Add handler as lambda function (just to show a different method)
  myWebServer.on("/version", HTTP_GET, []() {    
    EEPROM.get(0, fw_version);
    if (fw_version[0] == 0xFF) // Still not stored in EEPROM (first run)
      strcpy(fw_version, "0.0.0");
    String reply = "{\"version\":\"";
    reply += fw_version;
    reply += "\", \"newFirmwareInfoJSON\":\"";
    reply += fimwareInfo;
    reply += "\"}";

    // Send to client actual firmware version and address where to check if new firmware available
    myWebServer.send(200, "text/json", reply);
  });

  // set /setup and /edit page authentication
  // myWebServer.setAuthentication("admin", "admin");

  // Enable ACE FS file web editor and add FS info callback function
  myWebServer.enableFsCodeEditor(getFsInfo);

  // Start webserver
  myWebServer.begin();
  Serial.print(F("ESP Web Server started on IP Address: "));
  Serial.println(myIP);
  Serial.println(F("Open /setup page to configure optional parameters"));
  Serial.println(F("Open /edit page to view and edit files"));
  Serial.println(F("Open /update page to upload firmware and filesystem updates"));

  pinMode(LED_BUILTIN, OUTPUT);
}

///////////////////////////////////  LOOP  ///////////////////////////////////////
void loop() {
  myWebServer.run();
}