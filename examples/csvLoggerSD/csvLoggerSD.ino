#include <SD.h>
#include <FSWebServer.h>

// Timezone definition to get properly time from NTP server
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"
#include <time.h>

#define PIN_CS 14
#define PIN_SCK 13
#define PIN_MOSI 12
#define PIN_MISO 11


FSWebServer server(SD, 80, "myServer");
bool captiveRun = false;
struct tm ntpTime;
const char* basePath = "/csv";

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
bool startFilesystem(){
  if (SD.begin(PIN_CS)){
    server.printFileList(SD, "/", 2);
    return true;
  }
  else {
    Serial.println("ERROR on mounting filesystem. It will be formmatted!");
    ESP.restart();
  }
  return false;
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
  if (SD.exists(filename)) {
    file = SD.open(filename, "a");   // Append to existing file
  }
  else {
    file = SD.open(filename, "w");   // Create a new file
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


void setup() {
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);
  Serial.begin(115200);
  
  delay(1000);
  startFilesystem();

  // Create csv logs folder if not exists
  if (!SD.exists(basePath)) {
    SD.mkdir(basePath);
  }

  // Try to connect to WiFi (will start AP if not connected after timeout)
  if (!server.startWiFi(10000)) {
    Serial.println("\nWiFi not connected! Starting AP mode...");
    server.startCaptivePortal("ESP_AP", "123456789", "/setup");
    captiveRun = true;
  }

  // Enable ACE FS file web editor and add FS info callback fucntion
  server.enableFsCodeEditor();
  #ifdef ESP32
  server.setFsInfoCallback([](fsInfo_t* fsInfo) {
      fsInfo->totalBytes = SD.totalBytes();
      fsInfo->usedBytes = SD.usedBytes();
      fsInfo->fsName = "SD";
  });
  #endif

  // Start server
  server.begin();
  Serial.print(F("\nAsync ESP Web Server started on IP Address: "));
  Serial.println(server.getServerIP());
  Serial.println(F(
      "This is \"scvLoggerSdFat.ino\" example.\n"
      "Open /setup page to configure optional parameters.\n"
      "Open /edit page to view, edit or upload example or your custom webserver source files."
  ));

  // Set NTP servers
  #ifdef ESP8266
  configTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
  #elif defined(ESP32)
  configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
  #endif

  // Wait for NTP sync (with timeout)
  getUpdatedtime(5000);

  appenRow();
}

void loop() {
  server.handleClient();
  if (captiveRun)
    server.updateDNS();

  static uint32_t updateTime;
  if (millis()- updateTime > 30000) {
    updateTime = millis();
    appenRow();
  }
}
