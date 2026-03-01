#include <FS.h>
#include <LittleFS.h>
#include <FSWebServer.h>

FSWebServer server(LittleFS, 80, "myServer");
uint16_t testInt = 150;
float testFloat = 123.456f;
bool testBool = true;

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif
const uint8_t ledPin = LED_BUILTIN;

////////////////////////////////  Filesystem  /////////////////////////////////////////
bool startFilesystem() {
  if (LittleFS.begin()) {
    server.printFileList(LittleFS, "/", 1, Serial);    
    return true;
  } else {
    Serial.println("ERROR on mounting filesystem. It will be reformatted!");
    LittleFS.format();
    ESP.restart();
  }
  return false;
}

////////////////////////////  Load custom options //////////////////////////////////////
bool loadApplicationConfig() {
  if (LittleFS.exists(server.getConfiFileName())) {
    // Test "options" values
    server.getOptionValue("Test int variable", testInt);
    server.getOptionValue("Test float variable", testFloat);
    server.getOptionValue("Test bool variable", testBool);
    return true;
  }
  return false;
}

//---------------------------------------
void handleLed() {
  static int value = false;
  // http://xxx.xxx.xxx.xxx/led?val=1
  if (server.hasArg("val")) {
    value = server.arg("val").toInt();
    digitalWrite(ledPin, value);
  }
  String reply = "LED is now ";
  reply += value ? "ON" : "OFF";
  server.send(200, "text/plain", reply);
}


void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);
  delay(1000);
  if (startFilesystem()) {
    // Load application config options
    if (loadApplicationConfig()) {
      Serial.printf("Stored \"testInt\" value: %d\n", testInt);
      Serial.printf("Stored \"testFloat\" value: %3.3f\n", testFloat);
      Serial.printf("Stored \"testBool\" value: %s\n", testBool?"true":"false");
    }
  } else
    Serial.println("LittleFS error!");

  // Try to connect to WiFi (will start AP if not connected after timeout)
  if (!server.startWiFi(10000)) {
    Serial.println("\nWiFi not connected! Starting AP mode...");
    server.startCaptivePortal("ESP_AP", "123456789", "/setup");
  }

  // Add custom application options tab and set custom title
  server.addOptionBox("Custom options");
  server.addOption("Test int variable", testInt);
  server.addOption("Test float variable", (double)testFloat, 0.0, 100.0, 0.001);
  // add a helpful comment beneath the float slider
  server.addComment("Test float variable", "Use this to adjust the intensity (0-100)");

  // add a helpful comment direclty (boolean comment appears inline by default)
  server.addOption("Test bool variable", testBool, "Enable/disable feature");
  server.setSetupPageTitle("Simple ESP FS WebServer");

  // Enable ACE FS file web editor
  server.enableFsCodeEditor();

  // Custom endpoint handler
  server.on("/led", HTTP_GET, handleLed);

  // Start server
  server.begin();
  Serial.print(F("\nAsync ESP Web Server started on IP Address: "));
  Serial.println(server.getServerIP());
  Serial.println(F(
    "This is \"simpleServer.ino\" example.\n"
    "Open /setup page to configure optional parameters.\n"
    "Open /edit page to view, edit or upload example or your custom webserver source files."));
}

void loop() {
  // Handle client requests
  server.run();

  // Nothing to do here for this simple example
  delay(1);
}
