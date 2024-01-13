// Timezone definition to get properly time from NTP server
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"
#include <time.h>
#include <esp-fs-webserver.h>  // https://github.com/cotestatnt/esp-fs-webserver

#include <FS.h>
#include <LittleFS.h>

// Check board options and select the right partition scheme
#define FILESYSTEM LittleFS

struct tm ntpTime;
const char* basePath = "/csv";

#ifdef ESP8266
  ESP8266WebServer server(80);
#elif defined(ESP32)
  WebServer server(80);
#endif
FSWebServer myWebServer(FILESYSTEM, server);

////////////////////////////////  NTP Time  /////////////////////////////////////////
void getUpdatedtime(const uint32_t timeout) {
  uint32_t start = millis();
  do {
    time_t now = time(nullptr);
    ntpTime = *localtime(&now);
    delay(1);
  } while (millis() - start < timeout && ntpTime.tm_year <= (1970 - 1900));
}

////////////////////////////////  Filesystem  /////////////////////////////////////////
void startFilesystem(){
  // FILESYSTEM INIT
  if ( !FILESYSTEM.begin()){
    Serial.println("ERROR on mounting filesystem. It will be formmatted!");
    FILESYSTEM.format();
    ESP.restart();
  }
}

//////////////////////////// Append a row to csv file ///////////////////////////////////
bool appenRow() {
  getUpdatedtime(10);

  char filename[24];
  snprintf(filename, sizeof(filename),
    "%s/%04d_%02d_%02d.csv",
    basePath,
    ntpTime.tm_year + 1900,
    ntpTime.tm_mon + 1,
    ntpTime.tm_mday
  );

  File file;
  if (FILESYSTEM.exists(filename)) {
    file = FILESYSTEM.open(filename, "a");   // Append to existing file
  }
  else {
    file = FILESYSTEM.open(filename, "w");   // Create a new file
    file.println("timestamp, free heap, largest free block, connected, wifi strength");
  }

  if (file) {
    char timestamp[25];
    strftime(timestamp, sizeof(timestamp), "%c", &ntpTime);

    char row[64];
  #ifdef ESP32
      snprintf(row, sizeof(row), "%s, %d, %d, %s, %d",
        timestamp,
        heap_caps_get_free_size(0),
        heap_caps_get_largest_free_block(0),
        (WiFi.status() == WL_CONNECTED) ? "true" : "false",
        WiFi.RSSI()
      );
  #elif defined(ESP8266)
      uint32_t free;
      uint16_t max;
      ESP.getHeapStats(&free, &max, nullptr);
      snprintf(row, sizeof(row),
        "%s, %d, %d, %s, %d",
        timestamp, free, max,
        (WiFi.status() == WL_CONNECTED) ? "true" : "false",
        WiFi.RSSI()
      );
  #endif
    Serial.println(row);
    file.println(row);
    file.close();
    return true;
  }

  return false;
}


void setup(){
  Serial.begin(115200);

  // FILESYSTEM INIT
  startFilesystem();

  // Try to connect to stored SSID, start AP if fails after timeout
  char ssid[20];
  snprintf(ssid, sizeof(ssid), "ESP-%llX", ESP.getEfuseMac());
  myWebServer.setAP(ssid, "123456789");

  IPAddress myIP = myWebServer.startWiFi(15000);
  Serial.println("\n");

// Sync time with NTP
#ifdef ESP8266
  configTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
#elif defined(ESP32)
  configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
#endif
  getUpdatedtime(5000);    // Wait for NTP sync

  // Start webserver
  if (myWebServer.begin()) {
    Serial.print(F("ESP Web Server started on IP Address: "));
    Serial.println(myIP);
    Serial.println(F("Open /setup page to configure optional parameters"));
    Serial.println(F("Open /edit page to view and edit files"));
  }

  // Create csv log folder if not exists
  if (!FILESYSTEM.exists(basePath)) {
    FILESYSTEM.mkdir(basePath);
  }

}


void loop() {

  myWebServer.run();

  static uint32_t updateTime;
  if (millis()- updateTime > 30000) {
    updateTime = millis();
    appenRow();
  }
}