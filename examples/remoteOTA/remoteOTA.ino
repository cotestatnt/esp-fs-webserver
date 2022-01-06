#include <esp-fs-webserver.h>  // https://github.com/cotestatnt/esp-fs-webserver

#ifdef ESP8266
  #include <ESP8266HTTPClient.h>
  #include <ESP8266httpUpdate.h>
#elif defined(ESP32)
  #include <HTTPClient.h>
  #include <HTTPUpdate.h>
#endif
#include <EEPROM.h>            // For storing the firmware version

#include <FS.h>
#ifdef ESP8266
  #include <LittleFS.h>
  #define FILESYSTEM LittleFS
#elif defined(ESP32)
  #include <FFat.h>
  #define FILESYSTEM FFat
#endif

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// In order to set SSID and password open the /setup webserver page
// const char* ssid;
// const char* password;

uint8_t ledPin = LED_BUILTIN;
bool apMode = false;
char* hostname = "fsbrowser";
char fw_version[10] = {"0.0.0"};

#ifdef ESP8266
  ESP8266WebServer server(80);
#elif defined(ESP32)
  WebServer server(80);
#endif
FSWebServer myWebServer(FILESYSTEM, server);



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
    Serial.println("ERROR on mounting filesystem. It will be formmatted!");
    FILESYSTEM.format();
    ESP.restart();
  }
}


void handleLed() {
  // If new led state is specified - http://xxx.xxx.xxx.xxx/led?val=1
  if(myWebServer.webserver->hasArg("val")) {
    int value = myWebServer.webserver->arg("val").toInt();
    digitalWrite(ledPin, value);
  }
  // else simple toggle the actual state
  else {
    digitalWrite(ledPin, !digitalRead(ledPin));
  }
  String reply = "LED is now ";
  reply += digitalRead(ledPin) ? "OFF" : "ON";
  myWebServer.webserver->send(200, "text/plain", reply);
}

void handleUpdate() {
  if(myWebServer.webserver->hasArg("fwupdate")) {
    bool value = myWebServer.webserver->arg("fwupdate").toInt();
    String new_version = myWebServer.webserver->arg("version");
    String url = myWebServer.webserver->arg("url");
    String reply = "Firmware is going to be updated to version ";
    reply += new_version;
    myWebServer.webserver->send(200, "text/plain", reply );
    
    Serial.println(reply);
    Serial.println(url);
    
    if( value && WiFi.status() == WL_CONNECTED) {

      #ifdef ESP8266
      #define UPDATER ESPhttpUpdate
      // onProgress handling is missing with ESP32 library
      ESPhttpUpdate.onProgress([](int cur, int total){
          static uint32_t sendT;
          if(millis() - sendT > 1000){
              sendT = millis();
              Serial.printf("Updating %d of %d bytes...\n", cur, total);
          }
      });
      #elif defined(ESP32)
      #define UPDATER httpUpdate
      #endif

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
          strcpy(fw_version, new_version.c_str());
          EEPROM.put(0, fw_version);
          EEPROM.commit();     
          Serial.print("System will be restarted with the new version ");
          Serial.println(fw_version);   
          delay(1000);
          ESP.restart();                                  
          break;
      }
    }
  }
}

void handleVersion() {  
  EEPROM.get(0, fw_version);
  Serial.printf("\n\nFirmware version: %s\n", fw_version );
  if(fw_version != NULL) 
    myWebServer.webserver->send(200, "text/plain", fw_version);  
  else
    myWebServer.webserver->send(200, "text/plain", "0.0.0");
}

///////////////////////////////////  SETUP  ///////////////////////////////////////
void setup(){
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  EEPROM.begin(128);

  // FILESYSTEM INIT
  startFilesystem();

  // Try to connect to flash stored SSID, start AP if fails after timeout
  IPAddress myIP = myWebServer.startWiFi(15000, "ESP8266_AP", "123456789" );

  // Add custom page handlers to webserver
  myWebServer.addHandler("/led", HTTP_GET, handleLed);
  myWebServer.addHandler("/firmware_update", HTTP_GET, handleUpdate);
  myWebServer.addHandler("/version", HTTP_GET, handleVersion);

  /*
  // Add handler as lambda function
  myWebServer.addHandler("/version", HTTP_GET, []() {
    if(firmware_version.length())
      myWebServer.webserver->send(200, "text/plain", firmware_version);
  });
  */

  // Start webserver
  if (myWebServer.begin()) {
    Serial.print(F("ESP Web Server started on IP Address: "));
    Serial.println(myIP);
    Serial.println(F("Open /setup page to configure optional parameters"));
    Serial.println(F("Open /edit page to view and edit files"));
  }

  pinMode(LED_BUILTIN, OUTPUT);
}

///////////////////////////////////  LOOP  ///////////////////////////////////////
void loop() {
  myWebServer.run();

}