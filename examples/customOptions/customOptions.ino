#include <FS.h>
#include <LittleFS.h>
#include <FSWebServer.h>  //https://github.com/cotestatnt/esp-fs-webserver

// Timezone definition to get properly time from NTP server
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"
struct tm Time;

#define FILESYSTEM LittleFS
FSWebServer server(80, FILESYSTEM, "myserver");

// Define built-in LED if not defined by board (eg. generic dev boards)
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

#ifndef BOOT_PIN
#define BOOT_PIN    0
#endif

// Labels used in /setup webpage for options
#define LED_LABEL "The LED pin number"
#define BOOL_LABEL "A bool variable"
#define BOOL_LABEL2 "A second bool variable"
#define LONG_LABEL "A long variable"
#define FLOAT_LABEL "A float variable"
#define STRING_LABEL "A String variable"
#define DROPDOWN_LABEL "Days of week"
#define BRIGHTNESS_LABEL "Brightness"

// Test "options" values
uint8_t ledPin = LED_BUILTIN;
bool boolVar = true;
bool boolVar2 = false;
uint32_t longVar = 1234567890;
float floatVar = 15.51F;
String stringVar = "Test option String";

// Add a dropdown list box in /setup page
const char* days[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
uint8_t daySelected = 2;// Default to "Wednesday"
FSWebServer::DropdownList dayOfWeek{ DROPDOWN_LABEL, days, 7, daySelected}; 

// Add a slider in /setup page
FSWebServer::Slider brightness{ BRIGHTNESS_LABEL, 0.0, 100.0, 1.0, 50.0 };

static const char reload_btn_htm[] PROGMEM = R"EOF(
<div class="btn-bar">
  <a class="btn" id="reload-btn">Reload options</a>
</div>
)EOF";

static const char reload_btn_script[] PROGMEM = R"EOF(
/* Add click listener to button */
document.getElementById('reload-btn').addEventListener('click', reload);
function reload() {
  console.log('Reload configuration options');
  fetch('/reload')
  .then((response) => {
    if (response.ok) {
      openModal('Options loaded', 'Options was reloaded from configuration file');
      return;
    }
    throw new Error('Something goes wrong with fetch');
  })
  .catch((error) => {
    openModal('Error', 'Something goes wrong with your request');
  });
}
)EOF";


/////////// Callback: notify user when the configuration file is saved  /////////////
void onConfigSaved(const char* path) {
  Serial.printf("\n[Config] File salvato: %s\n", path);
}

////////////////////////////////  Filesystem  /////////////////////////////////////////
bool startFilesystem() {
  if (FILESYSTEM.begin()){
    server.printFileList(FILESYSTEM, "/", 2);
    return true;
  }
  else {
    Serial.println("ERROR on mounting filesystem. It will be reformatted!");
    FILESYSTEM.format();
    ESP.restart();
  }
  return false;
}

////////////////////  Load application options from filesystem  ////////////////////
bool loadOptions() {
  if (FILESYSTEM.exists(server.getConfiFileName())) {
    server.getOptionValue(LED_LABEL, ledPin);
    server.getOptionValue(BOOL_LABEL, boolVar);
    server.getOptionValue(BOOL_LABEL2, boolVar2);
    server.getOptionValue(LONG_LABEL, longVar);
    server.getOptionValue(FLOAT_LABEL, floatVar);
    server.getOptionValue(STRING_LABEL, stringVar);
    server.getDropdownSelection(dayOfWeek);    
    server.getSliderValue(brightness);
    server.closeSetupConfiguration();  // Close configuration to free resources

    Serial.println("\nThis are the current values stored: \n");
    Serial.printf("LED pin value: %d\n", ledPin);
    Serial.printf("Bool value: %s\n", boolVar ? "true" : "false");
    Serial.printf("Bool value2: %s\n", boolVar2 ? "true" : "false");
    Serial.printf("Long value: %u\n", longVar);
    Serial.printf("Float value: %3.2f\n", floatVar);
    Serial.printf("String value: %s\n", stringVar.c_str());
    Serial.printf("Dropdown selected value: %s\n", days[dayOfWeek.selectedIndex]);
    Serial.printf("Slider value: %3.2f\n\n", brightness.value);
    return true;
  }
  else
    Serial.println(F("Config file not exist"));
  return false;
}


////////////////////////////  HTTP Request Handlers  ////////////////////////////////////
void handleLoadOptions() {
  server.send(200, "text/plain", "Options loaded");
  loadOptions();
  Serial.println("Application option loaded after web request");
} 


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BOOT_PIN, INPUT_PULLUP);
  Serial.begin(115200);

  // FILESYSTEM INIT
  if (startFilesystem()){
    // Load configuration (if not present, default will be created when webserver will start)
    loadOptions();
  }

  // Default firmware version is set to compile time, but you can edit with a custom string
  // Custom firmware version -> Major.Minor.Build (build is set to compile date YYYYMMDDHHMM)
  String version = "1.0." + String(BUILD_TIMESTAMP);
  server.setFirmwareVersion(version);

  // Try to connect to WiFi (will start AP if not connected after timeout)
  if (!server.startWiFi(10000)) {
    Serial.println("\nWiFi not connected! Starting AP mode...");
    server.startCaptivePortal("ESP_AP", "123456789", "/setup");
  }

  // Add custom page handler
  server.on("/reload", HTTP_GET, handleLoadOptions);

  // Configure /setup page and start Web Server
  server.addOptionBox("My Options");
  server.addOption(BOOL_LABEL, boolVar);
  server.addOption(BOOL_LABEL2, boolVar2);
  server.addOption(LED_LABEL, ledPin);
  server.addOption(LONG_LABEL, longVar);
  server.addOption(FLOAT_LABEL, floatVar, 1.0, 100.0, 0.01);
  server.addOption(STRING_LABEL, stringVar);
  server.addDropdownList(dayOfWeek);
  server.addSlider(brightness);  
  server.addHTML(reload_btn_htm, "buttons", /*overwrite*/ false);
  server.addJavascript(reload_btn_script, "js", /*overwrite*/ false);

  // Enable ACE FS file web editor and add FS info callback function    
#ifdef ESP32
  server.enableFsCodeEditor([](fsInfo_t* fsInfo) {
    fsInfo->fsName = "LittleFS";
    fsInfo->totalBytes = LittleFS.totalBytes();
    fsInfo->usedBytes = LittleFS.usedBytes();
  });
#else
  // ESP8266 core support LittleFS by default
  server.enableFsCodeEditor();
#endif

  // set /setup and /edit page authentication
  server.setAuthentication("admin", "admin");

  // Inform user when config.json is saved via /edit or /upload
  server.setConfigSavedCallback(onConfigSaved);

  // Start server
  server.begin();
  Serial.print(F("\nESP Web Server started on IP Address: "));
  Serial.println(server.getServerIP());
  Serial.println(F(
      "\nThis is \"customOptions.ino\" example.\n"
      "Open /setup page to configure optional parameters.\n"
      "Open /edit page to view, edit or upload example or your custom webserver source files."
  ));
}

void loop() {
  server.handleClient();
  if (server.isAccessPointMode())
    server.updateDNS();

  // Keep BOOT_PIN pressed 5 seconds to clear application options
  static uint32_t buttonPressStart = 0;
  static bool buttonPressed = false;
  
  if (digitalRead(BOOT_PIN) == LOW) {
    if (!buttonPressed) {
      buttonPressed = true;
      buttonPressStart = millis();
    } 
    else if (millis() - buttonPressStart >= 5000) {
      Serial.println("\nClearing application options...");
      server.clearConfigFile();
      delay(1000);
      ESP.restart();
    }
  } else {
    buttonPressed = false;
  }

  // Nothing to do here, just a small delay for task yield
  delay(10);  
}
