#include <SD.h>
#include <esp-fs-webserver.h>  // https://github.com/cotestatnt/esp-fs-webserver

// Timezone definition to get properly time from NTP server
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"
#include <time.h>

#define PIN_CS 14
#define PIN_SCK 13
#define PIN_MOSI 12
#define PIN_MISO 11

// Check board options and select the right partition scheme
FSWebServer myWebServer(SD, 80, "hostname");
struct tm ntpTime;
const char* basePath = "/csv";


// This script will set page favicon using a base_64 encoded 32x32 pixel icon
static const char base64_favicon[] PROGMEM = R"string_literal(
var favIcon = "iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAACXBIWXMAAA7EAAAOxAGVKw4bAAABv0lEQVRYw+3XvWsUURTG4ScYsVEUJUtEEOxiYxTF0i6bSlBSaCXi32B" +
              "jpU0IEWyClR/Y2AiiLIiKIGhpY7DQRpQUimLEKEkUYjZrcwYuMhuZD4jFXJhi7rnve35z5szMHfLHCDpYQq/AcVENYwQLBRPXCtEJo3toFdTWApGVvVVCW0slMgMVAUpD1Al" +
              "QCqIKwPc6IKoAzNTxdFQB2BIQlSpRBaCw76ANHg1AA9AANAANwH8JMLARAAMYxRDe4GOyZjOOYhte4RN2YCe+xB4y89iHn/hcBGIIL5LPZRdXIrYfc1jFL/zGeYzH2snE51DM" +
              "TRX9zN/CCk5iVyTvoY27mMcebEo2G8P4ipeJz4XQHS4KMI+HyfnWuLJjeBKJDuTorke1dsf5c7wrs9FZw40+onEsh/ADbuNgxNoxfxbbo4rTZQC6uLmOcBjn4lb9wGLMDUb17" +
              "mAizI/08WhFfCkvOBdNmCZ8FKbHo7mycTrpD7iGbwH3fp3k90PXyVtwKYJXcQpPo9tH8Syu8gzG8Dges+y+j4V2BZf/8ZOyED++ue+C6UjUxWuciNhePIikq9H17b+0s3i" +
              "b0/29pOydfsn/AEofvJL5jAPxAAAAAElFTkSuQmCC";
var docHead = document.getElementsByTagName('head')[0];
var newLink = document.createElement('link');
newLink.rel = 'shortcut icon';
newLink.href = 'data:image/png;base64,' + favIcon;
docHead.appendChild(newLink);
)string_literal";


////////////////////////////////  Filesystem  /////////////////////////////////////////
void startFilesystem() {
  // FILESYSTEM INIT
  if (!SD.begin(PIN_CS)) {
    Serial.println("ERROR on mounting SD.");
  }
  myWebServer.printFileList(SD, Serial, "/", 2);
}

/*
* Getting FS info (total and free bytes) is strictly related to
* filesystem library used (SD, FFat, SPIFFS etc etc) and ESP framework
* ESP8266 FS implementation has methods for total and used bytes (only label is missing)
*/
#ifdef ESP32
void getFsInfo(fsInfo_t* fsInfo) {
	fsInfo->fsName = "SD";
	fsInfo->totalBytes = SD.totalBytes();
	fsInfo->usedBytes = SD.usedBytes();
}
#else
void getFsInfo(fsInfo_t* fsInfo) {
	fsInfo->fsName = "SD";
}
#endif

////////////////////////////////  NTP Time  /////////////////////////////////////////
void getUpdatedtime(const uint32_t timeout) {
  uint32_t start = millis();
  do {
    time_t now = time(nullptr);
    ntpTime = *localtime(&now);
    delay(1);
  } while (millis() - start < timeout && ntpTime.tm_year <= (1970 - 1900));
}


//////////////////////////// Append a row to csv file ///////////////////////////////////
bool appenRow() {
  getUpdatedtime(10);

  char filename[32];
  snprintf(filename, sizeof(filename),
           "%s/%04d_%02d_%02d_%02d.csv",
           basePath,
           ntpTime.tm_year + 1900,
           ntpTime.tm_mon + 1,
           ntpTime.tm_mday,
           ntpTime.tm_hour
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

  // SD init
  startFilesystem();
  // Create csv log folder if not exists
  if (!SD.exists(basePath)) {
    SD.mkdir(basePath);
  }

  // Try to connect to stored SSID, start AP if fails after timeout
  char ssid[20];
#ifdef ESP8266
  snprintf(ssid, sizeof(ssid), "ESP-%lX", ESP.getChipId());
#elif defined(ESP32)
  snprintf(ssid, sizeof(ssid), "ESP-%llX", ESP.getEfuseMac());
#endif

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

  // Configure /setup page and start Web Server
  myWebServer.addJavascript(base64_favicon, "favicon");

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
}


void loop() {

  myWebServer.run();

  static uint32_t updateTime;
  if (millis() - updateTime > 1000) {
    updateTime = millis();
    appenRow();
  }
}
