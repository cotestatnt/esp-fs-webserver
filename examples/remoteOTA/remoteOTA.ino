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
#include <FSWebServer.h>   // https://github.com/cotestatnt/esp-fs-webserver/

#define FILESYSTEM LittleFS
FSWebServer server(FILESYSTEM, 80);

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// In order to set SSID and password open the /setup webserver page
// const char* ssid;
// const char* password;

uint8_t ledPin = LED_BUILTIN;
bool apMode = false;

#ifdef ESP8266
String fimwareInfo = "https://raw.githubusercontent.com/cotestatnt/async-esp-fs-webserver/master/examples/remoteOTA/version-esp8266.json";
#elif defined(ESP32)
String fimwareInfo = "https://raw.githubusercontent.com/cotestatnt/async-esp-fs-webserver/master/examples/remoteOTA/version-esp32.json";
#endif

char fw_version[10] = {"0.0.0"};

//////////////////////////////  Firmware update /////////////////////////////////////////
void doUpdate(const char* url, const char* version) {

  #ifdef ESP8266
  #define UPDATER ESPhttpUpdate
  #elif defined(ESP32)
  #define UPDATER httpUpdate
  #if ESP_ARDUINO_VERSION_MAJOR > 2
  esp_task_wdt_config_t twdt_config = {
      .timeout_ms = 15*1000,
      .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,    // Bitmask of all cores
      .trigger_panic = false,
  };
  ESP_ERROR_CHECK(esp_task_wdt_reconfigure(&twdt_config));
  #else
  ESP_ERROR_CHECK(esp_task_wdt_init(15, 0));
  #endif
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
void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  Serial.printf("\nListing directory: %s\n", dirname);
  File root = fs.open(dirname, "r");
  if(!root){
    Serial.println("- failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println(" - not a directory");
    return;
  }
  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      if(levels){
        #ifdef ESP8266
        String path = file.fullName();
        path.replace(file.name(), "");
        #elif defined(ESP32)
        String path = file.path();
        #endif
        listDir(fs, path.c_str(), levels -1);
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


////////////////////////////  HTTP Request Handlers  ////////////////////////////////////
void handleLed() {
  // http://xxx.xxx.xxx.xxx/led?val=1
  if(server.hasArg("val")) {
    int value = server.arg("val").toInt();
    digitalWrite(ledPin, value);
  }

  String reply = "LED is now ";
  reply += digitalRead(ledPin) ? "OFF" : "ON";
  server.send(200, "text/plain", reply);
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
  if(server.hasArg("version") && server.hasArg("url")) {
    const char* new_version = server.arg("version").c_str();
    const char* url = server.arg("url").c_str();
    String reply = "Firmware is going to be updated to version ";
    reply += new_version;
    reply += " from remote address ";
    reply += url;
    reply += "<br>Wait 10-20 seconds and then reload page.";
    server.send(200, "text/plain", reply );
    Serial.println(reply);
    doUpdate(url, new_version);
  }
}

///////////////////////////////////  SETUP  ///////////////////////////////////////
void setup(){
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  EEPROM.begin(128);

  // FILESYSTEM INIT
  startFilesystem();
  
  // Try to connect to WiFi (will start AP if not connected after timeout)
  if (!server.startWiFi(10000)) {
    Serial.println("\nWiFi not connected! Starting AP mode...");
    server.startCaptivePortal("ESP_AP", "123456789", "/setup");
  }

  /*
  * Getting FS info (total and free bytes) is strictly related to
  * filesystem library used (LittleFS, FFat, SPIFFS etc etc) and ESP framework
  */
  #ifdef ESP32
  server.setFsInfoCallback([](fsInfo_t* fsInfo) {
    fsInfo->fsName = "LittleFS";
    fsInfo->totalBytes = LittleFS.totalBytes();
    fsInfo->usedBytes = LittleFS.usedBytes();
  });
  #endif

  // Enable ACE FS file web editor and add FS info callback function
  server.enableFsCodeEditor();

  // Add custom handlers to webserver
  server.on("/led", HTTP_GET, handleLed);
  server.on("/firmware_update", HTTP_GET, handleUpdate);

  // Add handler as lambda function (just to show a different method)
  server.on("/version", HTTP_GET, []() {
    server.getOptionValue("New firmware JSON", fimwareInfo);

    EEPROM.get(0, fw_version);
    if (fw_version[0] == 0xFF) // Still not stored in EEPROM (first run)
      strcpy(fw_version, "0.0.0");
    String reply = "{\"version\":\"";
    reply += fw_version;
    reply += "\", \"newFirmwareInfoJSON\":\"";
    reply += fimwareInfo;
    reply += "\"}";
    // Send to client actual firmware version and address where to check if new firmware available
    server.send(200, "text/json", reply);
  });

  // Configure /setup page and start Web Server
  server.addOptionBox("Remote Update");
  server.addOption("New firmware JSON", fimwareInfo);

  // Start server with built-in websocket event handler
  server.begin();
  Serial.print(F("ESP Web Server started on IP Address: "));
  Serial.println(server.getServerIP());
  Serial.println(F(
    "This is \"remoteOTA.ino\" example.\n"
    "Open /setup page to configure optional parameters.\n"
    "Open /edit page to view, edit or upload example or your custom webserver source files."
  ));
}

///////////////////////////////////  LOOP  ///////////////////////////////////////
void loop() {
  server.handleClient();
  if (server.isAccessPointMode())
    server.updateDNS();
  
  // This delay is required in order to avoid loopTask() WDT reset on ESP32
  delay(1);  
}