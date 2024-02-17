#include <esp-fs-webserver.h>   // https://github.com/cotestatnt/esp-fs-webserver
#include <FS.h>
#include <LittleFS.h>
#define FILESYSTEM LittleFS
#include "custom_html.h"

FSWebServer myWebServer(FILESYSTEM, 80);

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

#define BTN_SAVE  5

// Test "options" values
uint8_t ledPin = LED_BUILTIN;
bool boolVar = true;
uint32_t longVar = 1234567890;
float floatVar = 15.5F;
String stringVar = "Test option String";

// In order to show a dropdown list box in /setup page
// we need a list ef values and a variable to store the selected option
#define LIST_SIZE  7
const char* dropdownList[LIST_SIZE] = {
  "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
String dropdownSelected;

// Var labels (in /setup webpage)
#define LED_LABEL "The LED pin number"
#define BOOL_LABEL "A bool variable"
#define LONG_LABEL "A long variable"
#define FLOAT_LABEL "A float variable"
#define STRING_LABEL "A String variable"
#define DROPDOWN_LABEL "A dropdown listbox"

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


////////////////////  Load application options from filesystem  ////////////////////
bool loadOptions() {
  if (FILESYSTEM.exists(myWebServer.getConfigFilepath())) {
    // Config file will be opened on the first time we call this method
    myWebServer.getOptionValue(LED_LABEL, ledPin);
    myWebServer.getOptionValue(BOOL_LABEL, boolVar);
    myWebServer.getOptionValue(LONG_LABEL, longVar);
    myWebServer.getOptionValue(FLOAT_LABEL, floatVar);
    myWebServer.getOptionValue(STRING_LABEL, stringVar);
    myWebServer.getOptionValue(DROPDOWN_LABEL, dropdownSelected);
    // Close configuration file and release memory
    myWebServer.closeConfiguration(false);

    Serial.println("\nThis are the current values stored: \n");
    Serial.printf("LED pin value: %d\n", ledPin);
    Serial.printf("Bool value: %s\n", boolVar ? "true" : "false");
    Serial.printf("Long value: %d\n",longVar);
    Serial.printf("Float value: %d.%d\n", (int)floatVar, (int)(floatVar*1000)%1000);
    Serial.printf("String value: %s\n", stringVar.c_str());
    Serial.printf("Dropdown selected value: %s\n\n", dropdownSelected.c_str());
    return true;
  }
  else
    Serial.println(F("Config file not exist"));
  return false;
}

void saveOptions() {
  // Config file will be opened on the first time we call this method
  myWebServer.saveOptionValue(LED_LABEL, ledPin);
  myWebServer.saveOptionValue(BOOL_LABEL, boolVar);
  myWebServer.saveOptionValue(LONG_LABEL, longVar);
  myWebServer.saveOptionValue(FLOAT_LABEL, floatVar);
  myWebServer.saveOptionValue(STRING_LABEL, stringVar);
  myWebServer.saveOptionValue(DROPDOWN_LABEL, dropdownSelected);
  // Close config file and release memory
  myWebServer.closeConfiguration(true);
  Serial.println(F("Application options saved."));
}

////////////////////////////  HTTP Request Handlers  ////////////////////////////////////
void handleLoadOptions() {
  myWebServer.send(200, "text/plain", "Options loaded");
  loadOptions();
  Serial.println("Application option loaded after web request");
}


void setup() {
  Serial.begin(115200);
  pinMode(BTN_SAVE, INPUT_PULLUP);

  // FILESYSTEM INIT
  startFilesystem();

  // Try to connect to stored SSID, start AP if fails after timeout
  myWebServer.setAP("ESP_AP", "123456789");
  IPAddress myIP = myWebServer.startWiFi(15000);

  // Load configuration (if not present, default will be created when webserver will start)
  if (loadOptions())
    Serial.println(F("Application option loaded"));
  else
    Serial.println(F("Application options NOT loaded!"));

  // Add custom page handlers to webserver
  myWebServer.on("/reload", HTTP_GET, handleLoadOptions);

  // Configure /setup page and start Web Server
  myWebServer.addOptionBox("My Options");
  myWebServer.addOption(BOOL_LABEL, boolVar);
  myWebServer.addOption(LED_LABEL, ledPin);
  myWebServer.addOption(LONG_LABEL, longVar);
  myWebServer.addOption(FLOAT_LABEL, floatVar, 1.0, 100.0, 0.01);
  myWebServer.addOption(STRING_LABEL, stringVar);
  myWebServer.addDropdownList(DROPDOWN_LABEL, dropdownList, LIST_SIZE);
  myWebServer.addHTML(save_btn_htm, "buttons");
  myWebServer.addJavascript(button_script, "script");

  // set /setup and /edit page authentication
  // myWebServer.setAuthentication("admin", "admin");

  // Enable ACE FS file web editor and add FS info callback function
  myWebServer.enableFsCodeEditor(getFsInfo);

  myWebServer.begin();
  Serial.print(F("ESP Web Server started on IP Address: "));
  Serial.println(myIP);
  Serial.println(F("Open /setup page to configure optional parameters"));
  Serial.println(F("Open /edit page to view and edit files"));
  Serial.println(F("Open /update page to upload firmware and filesystem updates"));
}


void loop() {
  myWebServer.run();

  if (! digitalRead(BTN_SAVE)) {
    saveOptions();
    delay(1000);
  }

}
