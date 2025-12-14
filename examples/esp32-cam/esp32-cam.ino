
#include <FS.h>
#include <SD_MMC.h>
#include <FSWebServer.h>   // https://github.com/cotestatnt/esp-fs-webserver/
#include "esp_camera.h"
#include "soc/soc.h"          // Brownout error fix
#include "soc/rtc_cntl_reg.h" // Brownout error fix

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  #include "soc/soc_caps.h"
#endif

#define FILESYSTEM SD_MMC
FSWebServer server(80, FILESYSTEM);

// Local include files
#include "camera_pins.h"

uint16_t grabInterval = 0;  // Grab a picture every x seconds
uint32_t lastGrabTime = 0;

// Timezone definition to get properly time from NTP server
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"

// Struct for saving time datas (needed for time-naming the image files)
struct tm tInfo;

// Functions prototype
void listDir(const char *, uint8_t);
void setLamp(int);

// Grab a picture from CAM and store on SD or in flash
void getPicture(AsyncWebServerRequest *);
const char* getFolder = "/img";

///////////////////////////////////  SETUP  ///////////////////////////////////////
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detect

  // Flash LED setup
  pinMode(LAMP_PIN, OUTPUT);                      // set the lamp pin as output
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(LAMP_PIN, 1000, 8);
#else
  ledcSetup(lampChannel, pwmfreq, pwmresolution); // configure LED PWM channel
  ledcAttachPin(LAMP_PIN, lampChannel);
#endif
  setLamp(0);                                     // set default value

  Serial.begin(115200);
  Serial.println();

  // Try to connect to WiFi (will start AP if not connected after timeout)
  if (!server.startWiFi(10000)) {
    Serial.println("\nWiFi not connected! Starting AP mode...");
    server.startCaptivePortal("ESP32CAM_AP", "123456789", "/setup");
  }

  // Sync time with NTP
  configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");

  /*
   Init onboard SD filesystem (format if necessary)
   SD_MMC.begin(const char * mountpoint, bool mode1bit, bool format_if_mount_failed, int sdmmc_frequency, uint8_t maxOpenFiles)
   To avoid led glowing, set mode1bit = true (SD HS_DATA1 is tied to GPIO4, the same of on-board flash led)
  */
  if (!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_HIGHSPEED, 5)) {
    Serial.println("\nSD Mount Failed.\n");
  }

  if (!SD_MMC.exists(getFolder)) {
    if(SD_MMC.mkdir(getFolder))
      Serial.println("Dir created");
    else
      Serial.println("mkdir failed");
  }
  listDir(getFolder, 1);

  // Enable ACE FS file web editor and add FS info callback function
  server.enableFsCodeEditor();
  server.setFsInfoCallback([](fsInfo_t* fsInfo) {
    fsInfo->totalBytes = SD_MMC.totalBytes();
    fsInfo->usedBytes = SD_MMC.usedBytes();
  });

  // Add custom handlers to webserver
  server.on("/getPicture", getPicture);
  server.on("/setInterval", setInterval);

  // Start server with built-in websocket event handler
  server.begin();
  Serial.print(F("\nESP Web Server started on IP Address: "));
  Serial.println(server.getServerIP());
  Serial.println(F(
    "This is \"remoteOTA.ino\" example.\n"
    "Open /setup page to configure optional parameters.\n"
    "Open /edit page to view, edit or upload example or your custom webserver source files."
  ));

  // Init the camera module (according the camera_config_t defined)
  init_camera();
}

///////////////////////////////////  LOOP  ///////////////////////////////////////
void loop() {
  server.handleClient();
  if (server.isAccessPointMode())
    server.updateDNS();

  if (grabInterval) {
    if (millis() - lastGrabTime > grabInterval *1000) {
      lastGrabTime = millis();
      getPicture(nullptr);
    }
  }
}

//////////////////////////////////  FUNCTIONS//////////////////////////////////////
void setInterval(AsyncWebServerRequest *request) {
  if (request->hasArg("val")) {
    grabInterval = request->arg("val").toInt();
    Serial.printf("Set grab interval every %d seconds\n", grabInterval);
  }
  request->send(200, "text/plain", "OK");
}

// Lamp Control
void setLamp(int newVal) {
  if (newVal != -1) {
    // Apply a logarithmic function to the scale.
    int brightness = round((pow(2, (1 + (newVal * 0.02))) - 2) / 6 * pwmMax);
    ledcWrite(lampChannel, brightness);
    Serial.print("Lamp: ");
    Serial.print(newVal);
    Serial.print("%, pwm = ");
    Serial.println(brightness);
  }
}

// Send a picture taken from CAM to a Telegram chat
void getPicture(AsyncWebServerRequest *request) {

  // Take Picture with Camera;
  Serial.println("Camera capture requested");

  // Take Picture with Camera and store in ram buffer fb
  setLamp(100);
  delay(100);
  camera_fb_t *fb = esp_camera_fb_get();
  setLamp(0);

  if (!fb) {
    Serial.println("Camera capture failed");
    if (request != nullptr)
      request->send(500, "text/plain", "ERROR. Image grab failed");
    return;
  }

  // Keep files on SD memory, filename is time based (YYYYMMDD_HHMMSS.jpg)
  // Embedded filesystem is too small to keep all images, overwrite the same file
  char filename[20];
  time_t now = time(nullptr);
  tInfo = *localtime(&now);
  strftime(filename, sizeof(filename), "%Y%m%d_%H%M%S.jpg", &tInfo);

  char filePath[30];
  strcpy(filePath, getFolder);
  strcat(filePath, "/");
  strcat(filePath, filename);
  File file = SD_MMC.open(filePath, "w");
  if (!file) {
    Serial.println("Failed to open file in writing mode");
    if(request != nullptr)
      request->send(500, "text/plain", "ERROR. Image grab failed");
    return;
  }
  // size_t _jpg_buf_len = 0;
  // uint8_t *_jpg_buf = NULL;
  // bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
  file.write(fb->buf, fb->len);
  file.close();
  Serial.printf("Saved file to path: %s - %zu bytes\n", filePath, fb->len);

  // Clear buffer
  esp_camera_fb_return(fb);
  if (request != nullptr)
    request->send(200, "text/plain", filename);
}

// List all files saved in the selected filesystem
void listDir(const char *dirname, uint8_t levels) {
  uint32_t freeBytes = SD_MMC.totalBytes() - SD_MMC.usedBytes();
  Serial.print("\nTotal space: ");
  Serial.println(SD_MMC.totalBytes());
  Serial.print("Free space: ");
  Serial.println(freeBytes);

  Serial.printf("Listing directory: %s\r\n", dirname);
  File root = SD_MMC.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory\n");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory\n");
    return;
  }
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      if (levels)
        listDir(file.name(), levels - 1);
    }
    else {
      Serial.printf("|__ FILE: %s (%d bytes)\n",file.name(), file.size());
    }
    file = root.openNextFile();
  }
}
