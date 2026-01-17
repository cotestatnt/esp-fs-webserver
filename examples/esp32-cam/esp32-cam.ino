
#include <FS.h>
#include <SD_MMC.h>
#include <LittleFS.h>
#include <FSWebServer.h>   // https://github.com/cotestatnt/esp-fs-webserver/
#include "esp_camera.h"
#include "soc/soc.h"          // Brownout error fix
#include "soc/rtc_cntl_reg.h" // Brownout error fix
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  #include "soc/soc_caps.h"
#endif
// Local include files with camera pin definition and configuration
#include "camera_pins.h"
// Timezone definition to get properly time from NTP server
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3" 

// Set default file system type
#define USE_LITTLEFS          1
#define USE_SDMMC             2
#define FILESYSTEM_TYPE       USE_LITTLEFS

#if (FILESYSTEM_TYPE == USE_SDMMC)
  #define FILESYSTEM SD_MMC
#elif (FILESYSTEM_TYPE == USE_LITTLEFS)
  #define FILESYSTEM LittleFS
#endif

FSWebServer server(FILESYSTEM, 80, "esp32cam");

// Functions prototype
void setInterval();
void getPicture();

// Struct for saving time datas (needed for time-naming the image files)
struct tm tInfo;
uint16_t grabInterval = 0;  // Grab a picture every x seconds
uint32_t lastGrabTime = 0;
const char* getFolder = "/img";

///////////////////////////////////  SETUP  ///////////////////////////////////////
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detect

  // Flash LED setup
    pinMode(LAMP_PIN, OUTPUT); // set the lamp pin as output
  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    // Nuova API: ledcAttach(pin, freq, resolution)
    ledcAttach(LAMP_PIN, pwmfreq, pwmresolution);
  #else
    ledcSetup(lampChannel, pwmfreq, pwmresolution); // configure LED PWM channel
    ledcAttachPin(LAMP_PIN, lampChannel);
  #endif
    setLamp(0); // set default value

  Serial.begin(115200);
  Serial.println();

  // Try to connect to WiFi (will start AP if not connected after timeout)
  if (!server.startWiFi(10000)) {
    Serial.println("\nWiFi not connected! Starting AP mode...");
    server.startCaptivePortal("ESP32CAM_AP", "123456789", "/setup");
  }

  // Sync time with NTP
  configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");


#if (FILESYSTEM_TYPE == USE_SDMMC)
  /*
   Init onboard SD filesystem (format if necessary)
   SD_MMC.begin(const char * mountpoint, bool mode1bit, bool format_if_mount_failed, int sdmmc_frequency, uint8_t maxOpenFiles)
   To avoid led glowing, set mode1bit = true (SD HS_DATA1 is tied to GPIO4, the same of on-board flash led)
  */
  if (!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_HIGHSPEED, 5)) {
    Serial.println("\nSD Mount Failed.\n");
  }
  else if (!SD_MMC.exists(getFolder)) {
    if(SD_MMC.mkdir(getFolder))
      Serial.println("Dir created");
    else
      Serial.println("mkdir failed");
  }
#elif (FILESYSTEM_TYPE == USE_LITTLEFS)
  if (!LittleFS.begin()) {
    Serial.println("ERROR on mounting LittleFS. It will be formmatted!");
    LittleFS.format();
    ESP.restart();
  }
  else if (!LittleFS.exists(getFolder)) {
    if(LittleFS.mkdir(getFolder))
      Serial.println("Dir created");
    else
      Serial.println("mkdir failed");     
  }
  Serial.println("LittleFS mounted successfully");
#endif
  
  // List all files stored in filesystem
  server.printFileList(FILESYSTEM, "/", 1, Serial);

  // Enable ACE FS file web editor and add FS info callback function
  server.enableFsCodeEditor();

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
  server.run();

  if (grabInterval) {
    if (millis() - lastGrabTime > grabInterval *1000) {
      lastGrabTime = millis();
      getPicture();
    }
  }
}

//////////////////////////////////  FUNCTIONS//////////////////////////////////////
void setInterval() {
  if (server.hasArg("val")) {
    grabInterval = server.arg("val").toInt();
    Serial.printf("Set grab interval every %d seconds\n", grabInterval);
  }
  server.send(200, "text/plain", "OK");
}

// Lamp Control
void setLamp(int newVal) {
  if (newVal < 0) return;
  // Apply exponential scaling to have a better visual effect
  int brightness = round((pow(2, (1 + (newVal * 0.02))) - 2) / 6 * pwmMax);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  // ledcWrite(pin, value) with new API
  ledcWrite(LAMP_PIN, brightness);
#else
  ledcWrite(lampChannel, brightness);
#endif
  Serial.print("Lamp: ");
  Serial.print(newVal);
  Serial.print("%, pwm = ");
  Serial.println(brightness);
}

// Send a picture taken from CAM to a Telegram chat
void getPicture() {

  // Take Picture with Camera;
  Serial.println("Camera capture requested");

  // Take Picture with Camera and store in ram buffer fb
  setLamp(100);
  delay(100);
  camera_fb_t *fb = esp_camera_fb_get();
  setLamp(0);

  if (!fb) {
    Serial.println("Camera capture failed");    
    server.send(500, "text/plain", "ERROR. Image grab failed");
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
  File file = FILESYSTEM.open(filePath, "w");
  if (!file) {
    Serial.println("Failed to open file in writing mode");
    server.send(500, "text/plain", "ERROR. Image grab failed");
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
  server.send(200, "text/plain", filename);
}


