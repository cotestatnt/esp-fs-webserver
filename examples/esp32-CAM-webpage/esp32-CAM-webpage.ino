#include <WiFi.h>
#include <FS.h>
#include <SD_MMC.h>
#include <esp-fs-webserver.h> // https://github.com/cotestatnt/esp-fs-webserver

#include "esp_camera.h"
#include "soc/soc.h"          // Brownout error fix
#include "soc/rtc_cntl_reg.h" // Brownout error fix

// Local include files
#include "camera_pins.h"

FSWebServer myWebServer(SD_MMC, 80);

uint16_t grabInterval = 0;  // Grab a picture every x seconds
uint32_t lastGrabTime = 0;

// Timezone definition to get properly time from NTP server
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"

// Struct for saving time datas (needed for time-naming the image files)
struct tm tInfo;

// Functions prototype
void listDir(fs::FS &, const char *, uint8_t, bool);
void printHeapStats();
void setLamp(int);

// Grab a picture from CAM and store on SD or in flash
void getPicture();
const char* getFolder = "/img";

///////////////////////////////////  SETUP  ///////////////////////////////////////
void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detect

  // Flash LED setup
  pinMode(LAMP_PIN, OUTPUT);                      // set the lamp pin as output
  ledcSetup(lampChannel, pwmfreq, pwmresolution); // configure LED PWM channel
  setLamp(0);                                     // set default value
  ledcAttachPin(LAMP_PIN, lampChannel);           // attach the GPIO pin to the channel

  Serial.begin(115200);
  Serial.println();

  /*
   Init onboard SD filesystem (format if necessary)
   SD_MMC.begin(const char * mountpoint, bool mode1bit, bool format_if_mount_failed, int sdmmc_frequency, uint8_t maxOpenFiles)
   To avoid led glowindg, set mode1bit = true (SD HS_DATA1 is tied to GPIO4, the same of on-board flash led)
  */

  if (!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_HIGHSPEED, 5))
  {
    Serial.println("\nSD Mount Failed.\n");
  }

  if (!SD_MMC.exists(getFolder))
  {
    if(SD_MMC.mkdir(getFolder))
      Serial.println("Dir created");
    else
      Serial.println("mkdir failed");
  }
  listDir(getFolder, 0);

  // Try to connect to stored SSID, start AP if fails after timeout
  myWebServer.setAP("ESP_AP", "123456789");
  IPAddress myIP = myWebServer.startWiFi(15000);

  // Add custom page handlers to webserver
  myWebServer.on("/getPicture", getPicture);
  myWebServer.on("/setInterval", setInterval);
  
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
  Serial.println(F("Open /update page to upload firmware and filesystem updates"));

  // Sync time with NTP
  configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");

  // Init the camera module (accordind the camera_config_t defined)
  init_camera();
}

///////////////////////////////////  LOOP  ///////////////////////////////////////
void loop()
{
  myWebServer.run();
  if (grabInterval) {
    if (millis() - lastGrabTime > grabInterval *1000) {
      lastGrabTime = millis();
      getPicture();
    }
  }
}

//////////////////////////////////  FUNCTIONS//////////////////////////////////////

void setInterval() {  
  if(myWebServer.hasArg("val")) {
    grabInterval = 0;
    grabInterval = myWebServer.arg("val").toInt();
    Serial.printf("Set grab interval every %d seconds\n", grabInterval);
  }
  myWebServer.send(200, "text/plain", "OK");
}

// Lamp Control
void setLamp(int newVal)
{
  if (newVal != -1)
  {
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
void getPicture()
{  

  // Take Picture with Camera;
  Serial.println("Camera capture requested");

  // Take Picture with Camera and store in ram buffer fb
  setLamp(100);
  delay(100);
  camera_fb_t *fb = esp_camera_fb_get();
  setLamp(0);

  if (!fb)
  {
    Serial.println("Camera capture failed");
    if(webRequest)
      myWebServer.send(500, "text/plain", "ERROR. Image grab failed");
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
  if (!file)
  {
    Serial.println("Failed to open file in writing mode");
    if(webRequest)
      myWebServer.send(500, "text/plain", "ERROR. Image grab failed");
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
  if(webRequest.client())
    myWebServer.send(200, "text/plain", filename);
}

// List all files saved in the selected filesystem
void listDir(const char *dirname, uint8_t levels)
{
  uint32_t freeBytes = SD_MMC.totalBytes() - SD_MMC.usedBytes();
  Serial.print("\nTotal space: ");
  Serial.println(SD_MMC.totalBytes());
  Serial.print("Free space: ");
  Serial.println(freeBytes);

  Serial.printf("Listing directory: %s\r\n", dirname);
  File root = SD_MMC.open(dirname);
  if (!root)
  {
    Serial.println("- failed to open directory\n");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println(" - not a directory\n");
    return;
  }
  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.printf("  DIR : %s\n", file.name());
      if (levels)
        listDir(file.name(), levels - 1);
    }
    else
    {
      Serial.printf("  FILE: %s\tSIZE: %d", file.name(), file.size());
      Serial.println();
    }
    file = root.openNextFile();
  }
}
