#include <WebSocketsServer.h>   // https://github.com/Links2004/arduinoWebSockets
#include <esp-fs-webserver.h>   // https://github.com/cotestatnt/esp-fs-webserver

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
const char* hostname = "heap-chart";

// Timezone definition to get properly time from NTP server
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"
struct tm Time;

#ifdef ESP8266
  ESP8266WebServer server(80);
#elif defined(ESP32)
  WebServer server(80);
#endif

FSWebServer myWebServer(FILESYSTEM, server);
WebSocketsServer webSocket = WebSocketsServer(81);

////////////////////////////////   WebSocket Handler  /////////////////////////////
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
        Serial.printf("[%u] Disconnected!\n", num);
        break;
    case WStype_CONNECTED:
        {
          IPAddress ip = webSocket.remoteIP(num);
          Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
          // send message to client
          webSocket.sendTXT(num, "{\"Connected\": true}");
        }
        break;
    case WStype_TEXT:
        Serial.printf("[%u] get Text: %s\n", num, payload);
        // send message to client
        // webSocket.sendTXT(num, "message here");
        // send data to all connected clients
        // webSocket.broadcastTXT("message here");
        break;
    case WStype_BIN:
        Serial.printf("[%u] get binary length: %u\n", num, length);
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

////////////////////////////////  WiFi  /////////////////////////////////////////
IPAddress startWiFi(){
  IPAddress myIP;
  Serial.printf("Connecting to %s\n", WiFi.SSID().c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  // WiFi.begin(ssid, password);  
  while (WiFi.status() != WL_CONNECTED ){
    delay(500);
    Serial.print(".");
  }

  if(WiFi.status() == WL_CONNECTED) {
    myIP = WiFi.localIP();
    Serial.print(F("\nConnected! IP address: "));
    Serial.println(myIP);

    // Set hostname, timezone and NTP servers
  #ifdef ESP8266
    WiFi.hostname(hostname);
    configTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
  #elif defined(ESP32)
    WiFi.setHostname(hostname);
    configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
  #endif

    // Sync time with NTP. Blocking, but with timeout (0 == no timeout)
    getUpdatedtime(10000);
    char buffer[30];
    strftime (buffer, 30, "%d/%m/%Y - %X", &Time);
    Serial.printf("Synced time: %s\n", buffer);

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


void setup(){
  Serial.begin(115200);

  // FILESYSTEM INIT
  startFilesystem();

  // WiFi INIT
  IPAddress myIP = startWiFi();

  // Start WebSocket server on port 81
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

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
  webSocket.loop();

  if(WiFi.status() == WL_CONNECTED) {
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
    StaticJsonDocument<1024> doc;
    doc["addPoint"] = true;
    doc["timestamp"] = now;
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
    webSocket.broadcastTXT(msg);
  }

}
