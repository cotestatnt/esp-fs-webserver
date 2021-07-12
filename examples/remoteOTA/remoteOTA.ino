
#include <esp-fs-webserver.h>  // https://github.com/cotestatnt/esp-fs-webserver

#include <HTTPClient.h>
#include <HTTPUpdate.h>
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
const char* hostname = "fsbrowser";

String firmware_version;

#ifdef ESP8266
  ESP8266WebServer server(80);
#elif defined(ESP32)
  WebServer server(80);
#endif
FSWebServer myWebServer(FILESYSTEM, server);

////////////////////////////////  WiFi  /////////////////////////////////////////
IPAddress startWiFi(){
  IPAddress myIP;
  Serial.printf("Connecting to %s\n", WiFi.SSID().c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  // WiFi.begin(ssid, password);
  uint32_t startTime = millis();
  while (WiFi.status() != WL_CONNECTED ){
    delay(500);
    Serial.print(".");

    // If no connection (or specifically activated) go in Access Point mode
    if( millis() - startTime > 10000 || apMode ) {
      WiFi.mode(WIFI_AP);
      WiFi.softAP("ESP8266_AP", "123456789");
      myIP = WiFi.softAPIP();
      Serial.print(F("\nAP mode.\nServer IP address: "));
      Serial.println(myIP);
      break;
    }
  }

  if(WiFi.status() == WL_CONNECTED) {
    myIP = WiFi.localIP();
    Serial.print(F("\nConnected! IP address: "));
    Serial.println(myIP);

    // Set hostname
  #ifdef ESP8266
    WiFi.hostname(hostname);
  #elif defined(ESP32)
    WiFi.setHostname(hostname);
  #endif

    // Start MDNS responder
    if (MDNS.begin(hostname)) {
      Serial.println(F("MDNS responder started."));
      Serial.printf("You should be able to connect with address\t http://%s.local/\n", hostname);
      // Add service to MDNS-SD
      MDNS.addService("http", "tcp", 80);
    }
  }
  return myIP;
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
    String version = myWebServer.webserver->arg("version");
    String url = myWebServer.webserver->arg("url");
    String reply = "Firmware is going to be updated to version ";
    reply += version;
    myWebServer.webserver->send(200, "text/plain", reply );
    Serial.println(reply);
    Serial.println(url);
    if( value && WiFi.status() == WL_CONNECTED) {
      WiFiClientSecure client;
      client.setInsecure();
      httpUpdate.rebootOnUpdate(false);
      httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
      httpUpdate.setLedPin(LED_BUILTIN, LOW);
      t_httpUpdate_return ret = httpUpdate.update(client, url, version);

      switch (ret) {
        case HTTP_UPDATE_FAILED:
          Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
          break;

        case HTTP_UPDATE_NO_UPDATES:
          Serial.println("HTTP_UPDATE_NO_UPDATES");
          break;

        case HTTP_UPDATE_OK:
          Serial.println("HTTP_UPDATE_OK");
          firmware_version = version.c_str();
          EEPROM.put(0, firmware_version);
          EEPROM.commit();
          delay(1000);
          ESP.restart();
          break;
      }

    }
  }
}


void setup(){
  Serial.begin(115200);
  EEPROM.begin(128);

  // FILESYSTEM INIT
  startFilesystem();

  // WiFi INIT
  IPAddress myIP = startWiFi();

  EEPROM.get(0, firmware_version);
  Serial.printf("\n\nFirmware version: %s\n", firmware_version.c_str() );

  // Add custom page handlers to webserver
  myWebServer.addHandler("/led", HTTP_GET, handleLed);
  myWebServer.addHandler("/firmware_update", HTTP_GET, handleUpdate);
  // Add handler as lambda function
  myWebServer.addHandler("/version", HTTP_GET, []() {
    myWebServer.webserver->send(200, "text/plain", firmware_version);
  });

  // Start webserver
  if (myWebServer.begin()) {
    Serial.print(F("ESP Web Server started on IP Address"));
    Serial.println(myIP);
    Serial.println(F("Open /setup page to configure optional parameters"));
    Serial.println(F("Open /edit page to view and edit files"));
  }

  pinMode(LED_BUILTIN, OUTPUT);
}


void loop() {
  myWebServer.run();

  if(WiFi.status() == WL_CONNECTED) {
	#ifdef ESP8266
    MDNS.update();
	#endif
  }
}
