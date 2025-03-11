#include <esp-fs-webserver.h>   // https://github.com/cotestatnt/esp-fs-webserver
#include <FS.h>
#include <LittleFS.h>
#define FILESYSTEM LittleFS

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// In order to set SSID and password open the /setup webserver page
// const char* ssid;
// const char* password;
const char* hostname = "heap-chart";

// Timezone definition to get properly time from NTP server
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"
struct tm ntpTime;

FSWebServer myWebServer(FILESYSTEM, 80);
/////////////////////////   WebSocket callbck functions  //////////////////////////
void webSocketConnected(uint8_t num) {
  IPAddress ip = myWebServer.getWebSocketServer()->remoteIP(num);
  Serial.printf("[%u] Remote client connected %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
  // send message to client
  myWebServer.sendWebSocket(num, "{\"Connected\": true}");
}

void webSocketMessage(uint8_t num, uint8_t* payload, size_t len) {
  Serial.printf("[%u] WS get text(%d): %s\n", num, len, payload);
}

////////////////////////////////  NTP Time  /////////////////////////////////////
void getUpdatedtime(const uint32_t timeout)
{
  uint32_t start = millis();
  Serial.print("Sync time...");
  while (millis() - start < timeout  && ntpTime.tm_year <= (1970 - 1900)) {
    time_t now = time(nullptr);
    ntpTime = *localtime(&now);
    delay(5);
  }
  Serial.println(" done.");
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

void setup() {
  Serial.begin(115200);

  // FILESYSTEM INIT
  startFilesystem();

  // Try to connect to stored SSID, start AP if fails after timeout
  myWebServer.setAP("ESP_AP", "123456789");
  IPAddress myIP = myWebServer.startWiFi(15000);

  // Sync time with NTP
#ifdef ESP8266
  configTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
#elif defined(ESP32)
  configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
#endif
  getLocalTime(&ntpTime, 5000);  // Wait for NTP sync

  // set /setup and /edit page authentication
  // myWebServer.setAuthentication("admin", "admin");

  // Enable ACE FS file web editor and add FS info callback function
  myWebServer.enableFsCodeEditor(getFsInfo);

  // Enbable built-in WebSocket server and set callback functions
  myWebServer.enableWebsocket(81, webSocketMessage, webSocketConnected);

  // Start webserver
  myWebServer.begin();
  Serial.print(F("ESP Web Server started on IP Address: "));
  Serial.println(myIP);
  Serial.println(F("Open /setup page to configure optional parameters"));
  Serial.println(F("Open /edit page to view and edit files"));
  pinMode(LED_BUILTIN, OUTPUT);
}


void loop() {
  myWebServer.run();

  // Simula operazioni di allocazione/deallocazione con std::vector<uint8_t>
  static std::vector<std::vector<uint8_t>> buffers;

  if (random(0, 2)) { // Allocazione casuale
    size_t newSize = random(1000, 10000); // Dimensione casuale
    buffers.push_back(std::vector<uint8_t>(newSize, 0xFF)); // Riempie con dati fittizi
  } else if (!buffers.empty()) { // Deallocazione casuale
    buffers.erase(buffers.begin() + random(0, buffers.size())); // Rimuove un blocco casuale
  }

  // Send ESP system time (epoch) and heap stats to WS clients
  static uint32_t sendToClientTime;
  if (millis() - sendToClientTime >= 5000 ) {
    sendToClientTime = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

    JsonDocument doc;
    doc["timestamp"] = time(nullptr);
#ifdef ESP32
    doc["totalHeap"] = heap_caps_get_free_size(0);
    doc["maxBlock"]  =  heap_caps_get_largest_free_block(0);
#elif defined(ESP8266)
    uint32_t free;
    uint16_t max;
    ESP.getHeapStats(&free, &max, nullptr);
    doc["totalHeap"] = free;
    doc["maxBlock"]  =  max;
#endif
    String msg;
    serializeJson(doc, msg);
    myWebServer.broadcastWebSocket(msg);
  }

}