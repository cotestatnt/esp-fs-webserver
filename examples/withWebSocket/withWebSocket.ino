#include <Arduino.h>

#include <WebSocketsServer.h>   // https://github.com/Links2004/arduinoWebSockets
#include <esp-fs-webserver.h>   // https://github.com/cotestatnt/esp-fs-webserver


#include <FS.h>
#include <LittleFS.h>
#define FILESYSTEM LittleFS

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

FSWebServer myWebServer(FILESYSTEM, 80);
WebSocketsServer webSocket = WebSocketsServer(81);

// In order to set SSID and password open the /setup webserver page
// const char* ssid;
// const char* password;

bool apMode = false;
char const* hostname = "fsbrowser";

// Test "config" values
String option1 = "Test option String";
uint32_t option2 = 1234567890;
uint8_t ledPin = LED_BUILTIN;

// Timezone definition to get properly time from NTP server
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"
struct tm Time;


////////////////////////////////   WebSocket Handler  /////////////////////////////
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
        log_info("[%u] Disconnected!\n", num);
        break;
    case WStype_CONNECTED:
        {
          IPAddress ip = webSocket.remoteIP(num);
          log_info("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
          // send message to client
          webSocket.sendTXT(num, "{\"Connected\": true}");
        }
        break;
    case WStype_TEXT:
        log_info("[%u] get Text: %s\n", num, payload);
        // send message to client
        // webSocket.sendTXT(num, "message here");

        // send data to all connected clients
        // webSocket.broadcastTXT("message here");
        break;
    case WStype_BIN:
        log_info("[%u] get binary length: %u\n", num, length);
        break;
    default:
        break;
  }

}

////////////////////////////////  NTP Time  /////////////////////////////////////
void getUpdatedtime(const uint32_t timeout)
{
  uint32_t start = millis();
  log_info("Sync time...");
  while (millis() - start < timeout  && Time.tm_year <= (1970 - 1900)) {
    time_t now = time(nullptr);
    Time = *localtime(&now);
    delay(5);
  }
  log_info(" done.");
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


////////////////////  Load and save application configuration from filesystem  ////////////////////
void saveApplicationConfig(){
  StaticJsonDocument<1024> doc;
  File file = FILESYSTEM.open("/config.json", "w");
  doc["Option 1"] = option1;
  doc["Option 2"] = option2;
  doc["AP mode"] = apMode;
  doc["LED Pin"] = ledPin;
  serializeJsonPretty(doc, file);
  file.close();
  delay(1000);
  ESP.restart();
}

void loadApplicationConfig() {
  StaticJsonDocument<1024> doc;
  File file = FILESYSTEM.open("/config.json", "r");
  if (file) {
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (!error) {
      Serial.print(F("Deserializing JSON.."));
      apMode = doc["AP mode"];
      option1 = doc["Option 1"].as<String>();
      option2 = doc["Option 2"];
      ledPin = doc["LED Pin"];
    }
    else {
      Serial.print(F("Failed to deserialize JSON. File could be corrupted"));
      Serial.println(error.c_str());
      saveApplicationConfig();
    }
  }
  else {
    saveApplicationConfig();
    Serial.println(F("New file created with default values"));
  }
}

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


void setup(){

#if DEBUG_MODE_WS
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.setDebugOutput(true);
#endif

  // FILESYSTEM INIT
  startFilesystem();

  // Load configuration (if not present, default will be created)
  loadApplicationConfig();

  // Try to connect to stored SSID, start AP if fails after timeout
  myWebServer.setAP("ESP_AP", "");
  IPAddress myIP = myWebServer.startWiFi(15000);

  // Start WebSocket server on port 81
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Configure /setup page and start Web Server
  myWebServer.addOption("AP mode", apMode);
  myWebServer.addOption("LED Pin", ledPin);
  myWebServer.addOption("Option 1", option1.c_str());
  myWebServer.addOption("Option 2", option2);
  
  // set /setup and /edit page authentication
  // myWebServer.setAuthentication("admin", "admin");

  // Enable ACE FS file web editor and add FS info callback function
  myWebServer.enableFsCodeEditor(getFsInfo);
  
  // Add custom page handlers
  myWebServer.on("/led", HTTP_GET, handleLed);

  myWebServer.begin();
  Serial.print(F("ESP Web Server started on IP Address: "));
  Serial.println(myIP);
  Serial.println(F("Open /setup page to configure optional parameters"));
  Serial.println(F("Open /edit page to view and edit files"));
  Serial.println(F("Open /update page to upload firmware and filesystem updates"));

#ifdef ESP8266    
    configTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
#elif defined(ESP32)    
    configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
#endif

  pinMode(LED_BUILTIN, OUTPUT); 
}


void loop() {
  myWebServer.run();
  webSocket.loop();

  // Send ESP system time (epoch) to WS client
  static uint32_t sendToClientTime;
  if (millis() - sendToClientTime > 1000 ) {
    sendToClientTime = millis();
    time_t now = time(nullptr);
    char buffer[50];
    snprintf (buffer, sizeof(buffer), "{\"esptime\": %d}", (int)now);
    webSocket.broadcastTXT(buffer);
  }
}