#if defined(ESP8266)
#include <ESP8266mDNS.h>
#elif defined(ESP32)
#include <ESPmDNS.h>
#endif
#include <FS.h>
#include <LittleFS.h>
#include <FSWebServer.h>   // https://github.com/cotestatnt/esp-fs-webserver/

#define FILESYSTEM LittleFS
FSWebServer server(80, FILESYSTEM);

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// In order to set SSID and password open the /setup webserver page
// const char* ssid;
// const char* password;
const char* hostname = "heap-chart";

// Timezone definition to get properly time from NTP server
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"
struct tm Time;

////////////////////////////////   WebSocket Handler  /////////////////////////////
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  switch (type) {
    case WS_EVT_DISCONNECT:
      Serial.print("WebSocket client disconnected!\n");
      break;
    case WS_EVT_CONNECT:  {
        IPAddress ip = client->remoteIP();
        Serial.printf("WebSocket client %d.%d.%d.%d connected.\n", ip[0], ip[1], ip[2], ip[3]);
        client->printf("%s", "{\"Connected\": true}");
      }
      break;
    default:
        break;
  }
}


////////////////////////////////  NTP Time  /////////////////////////////////////
void getUpdatedtime(const uint32_t timeout)
{
  uint32_t start = millis();
  Serial.print("Sync time...");
  while (millis() - start < timeout  && Time.tm_year <= (1970 - 1900)) {
    time_t now = time(nullptr);
    Time = *localtime(&now);
    delay(5);
  }
  Serial.println(" done.");
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



void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);

  // FILESYSTEM INIT
  startFilesystem();

  // Try to connect to WiFi (will start AP if not connected after timeout)
  if (!server.startWiFi(10000)) {
    Serial.println("\nWiFi not connected! Starting AP mode...");
    server.startCaptivePortal("ESP_AP", "123456789", "/setup");
  }

  // Enable ACE FS file web editor and add FS info callback function
  server.enableFsCodeEditor();

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

  // Start server with custom websocket event handler
  server.begin(onWsEvent);
  Serial.print(F("ESP Web Server started on IP Address: "));
  Serial.println(server.getServerIP());
  Serial.println(F(
    "This is \"highcharts.ino\" example.\n"
    "Open /setup page to configure optional parameters.\n"
    "Open /edit page to view, edit or upload example or your custom webserver source files."
  ));

  // Start MDNS responder
  if (WiFi.status() == WL_CONNECTED) {
    // Set hostname
#ifdef ESP8266
    WiFi.hostname(hostname);
#elif defined(ESP32)
    WiFi.setHostname(hostname);
#endif
    if (MDNS.begin(hostname)) {
      Serial.println(F("MDNS responder started."));
      Serial.printf("You should be able to connect with address\t http://%s.local/\n", hostname);
      // Add service to MDNS-SD
      MDNS.addService("http", "tcp", 80);
    }
  }
}


void loop() {
  server.handleClient();
  if (server.isAccessPointMode())
    server.updateDNS();

  if (WiFi.status() == WL_CONNECTED) {
#ifdef ESP8266
    MDNS.update();
#endif
  }

  // Send ESP system time (epoch) and heap stats to WS client
  static uint32_t sendToClientTime;
  if (millis() - sendToClientTime > 1000 ) {
    sendToClientTime = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

    time_t now = time(nullptr);
    CJSON::Json doc;
    doc.setBool("addPoint", true);
    doc.setNumber("timestamp", (double)now);
#ifdef ESP32
    doc.setNumber("totalHeap", (double)heap_caps_get_free_size(0));
    doc.setNumber("maxBlock", (double)heap_caps_get_largest_free_block(0));
#elif defined(ESP8266)
    uint32_t free;
    uint32_t max;
    ESP.getHeapStats(&free, &max, nullptr);
    doc.setNumber("totalHeap", (double)free);
    doc.setNumber("maxBlock", (double)max);
#endif
    String msg = doc.serialize();
    server.broadcastWebSocket(msg);
  }

}