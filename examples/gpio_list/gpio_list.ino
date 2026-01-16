#include <FS.h>
#include <LittleFS.h>
#include <FSWebServer.h>   // https://github.com/cotestatnt/esp-fs-webserver/

#define FILESYSTEM LittleFS
FSWebServer server(80, FILESYSTEM, 80);
  
#ifndef LED_BUILTIN
  #define LED_BUILTIN 2 // Pin for built-in LED on ESP32  
#endif

#ifndef BOOT_PIN
  #define BOOT_PIN 0 // Pin for BOOT button on ESP32
#endif  

// Define a struct for store all info about each gpio
struct gpio_type {
  const char* type;
  const char* label;
  int pin;
  bool level;
};

// Define an array of struct GPIO and initialize with values

/* ESP8266 - Wemos D1-mini */
/*
  gpio_type gpios[] = {
  {"input", "INPUT 5", D5},
  {"input", "INPUT 6", D6},
  {"input", "INPUT 7", D7},
  {"output", "OUTPUT 2", D2},
  {"output", "OUTPUT 3", D3},
  {"output", "LED BUILTIN", LED_BUILTIN} // Led ON with signal LOW usually
  };
*/

/* ESP32 - NodeMCU-32S */
gpio_type gpios[] = {
  {"input", "INPUT 0", BOOT_PIN},
  {"input", "INPUT 1", 19},
  {"input", "INPUT 2", 21},
  {"output", "OUTPUT 1", 4},
  {"output", "OUTPUT 2", 5},
  {"output", "LED BUILTIN", LED_BUILTIN} // Led ON with signal LOW usually
};


/////////////////////////   WebSocket event callback////////////////////////////////   WebSocket Handler  /////////////////////////////
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:{
        IPAddress ip = server.getWebSocketServer()->remoteIP(num);
        server.getWebSocketServer()->sendTXT(num, "{\"Connected\": true}");
        Serial.printf("Hello client #%d [%s]\n", (int)num, ip.toString().c_str());
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] got Text: %s\n", num, payload);   // Got text message from a client
      if (payload[0] == '{')
        parseMessage((const char*)payload);
      break;
    case WStype_BIN:
      Serial.printf("[%u] got binary length: %u\n", num, length); // Got binary message from a client
      break;
    default:
      break;
  }
}


void parseMessage(const String json) {
  CJSON::Json doc;
  
  if (doc.parse(json)) {
    String cmd;
    if (doc.getString("cmd", cmd)) {
      // If this is a "writeOut" command, set the pin level to value
      if (cmd == "writeOut") {
        // getNumber returns double, so use double and cast to int
        double pin, level;
        if (doc.getNumber("pin", pin) && doc.getNumber("level", level)) {
          // Find the gpio in the array and set the level
          for (gpio_type &gpio : gpios) {
            if (gpio.pin == (int)pin) {
              Serial.printf("Set pin %d to %d\n", (int)pin, (int)level);
              gpio.level = (int)level;
              digitalWrite((int)pin, (int)level);
              // Update all clients with new gpio list
              updateGpioList();
              return;
            }
          }
        }
      }
    }
  } else {
    Serial.println(F("Failed to parse JSON message"));
  }
}

void updateGpioList() {
  // Build JSON array using CJSON::Json with nesting support
  CJSON::Json doc;
  doc.createArray();
  // Iterate the array of GPIO struct and add each as JSON object
  for (gpio_type &gpio : gpios) {
    CJSON::Json item;
    item.createObject();
    item.setString("type", String(gpio.type));
    item.setNumber("pin", gpio.pin);
    item.setString("label", String(gpio.label));
    item.setBool("level", gpio.level);
    doc.add(item);
  }
  // Serialize JSON document to string
  String json = doc.serialize();

  // Update client via websocket
  server.broadcastWebSocket(json);
  server.send(200, "text/plain", json);
}

bool updateGpioState() {
  // Iterate the array of GPIO struct and check level of inputs
  for (gpio_type &gpio : gpios) {
    if (strcmp(gpio.type, "input") == 0) {
      // Input value != from last read
      if (digitalRead(gpio.pin) != gpio.level) {
        gpio.level = digitalRead(gpio.pin);
        return true;
      }
    }
  }
  return false;
}



void setup() {
  Serial.begin(115200);

  // FILESYSTEM initialization
  if (!FILESYSTEM.begin()) {
    Serial.println("ERROR on mounting filesystem.");
    //FILESYSTEM.format();
    ESP.restart();
  }

  // Try to connect to WiFi (will start AP if not connected after timeout)
  if (!server.startWiFi(10000)) {
    Serial.println("\nWiFi not connected! Starting AP mode...");
    server.startCaptivePortal("ESP_AP", "123456789", "/setup");
  }

  // Enable ACE FS file web editor and add FS info callback function
  server.enableFsCodeEditor();

  /*
  * Getting FS info (total and free bytes) is strictly related to
  * filesystem library used (LittleFS, FFat, SPIFFS etc etc) and ESP framework
  */
  #ifdef ESP32
  server.setFsInfoCallback([](fsInfo_t* fsInfo) {
	  fsInfo->fsName = "LittleFS";
	  fsInfo->totalBytes = LittleFS.totalBytes();
	  fsInfo->usedBytes = LittleFS.usedBytes();  
  });
  #endif

  // Add custom page handlers
  server.on("/getGpioList", HTTP_GET, [](){ updateGpioList(); });

  // Start server with custom websocket event handler
  server.begin(webSocketEvent);
  Serial.print(F("ESP Web Server started on IP Address: "));
  Serial.println(server.getServerIP());
  Serial.println(F(
    "This is \"gpio_list.ino\" example.\n"
    "Open /setup page to configure optional parameters.\n"
    "Open /edit page to view, edit or upload example or your custom webserver source files."
  ));

  // GPIOs configuration
  for (gpio_type &gpio : gpios) {
    if (strcmp(gpio.type, "input") == 0)
        pinMode(gpio.pin, INPUT_PULLUP);
    else
      pinMode(gpio.pin, OUTPUT);
  }
}

void loop() {
  server.run();  // Handle client requests

  // True on pin state change
  if (updateGpioState()) {
    updateGpioList();   // Push new state to web clients via websocket
  }
}