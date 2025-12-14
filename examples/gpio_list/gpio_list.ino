#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <FSWebServer.h>   // https://github.com/cotestatnt/esp-fs-webserver/

#define FILESYSTEM LittleFS
FSWebServer server(80, FILESYSTEM);

// Define a struct for store all info about each gpio
struct gpio_type {
  const char* type;
  const char* label;
  int pin;
  bool level;
};

// Define an array of struct GPIO and initialize with values

/* (ESP32-C3) */
/*
gpio_type gpios[NUM_GPIOS] = {
  {"input", "INPUT 2", 2},
  {"input", "INPUT 4", 4},
  {"input", "INPUT 5", 5},
  {"output", "OUTPUT 6", 6},
  {"output", "OUTPUT 7", 7},
  {"output", "LED BUILTIN", 3} // Led ON with signal HIGH
};
*/

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
  {"input", "INPUT 18", 18},
  {"input", "INPUT 19", 19},
  {"input", "INPUT 21", 21},
  {"output", "OUTPUT 4", 4},
  {"output", "OUTPUT 5", 5},
  {"output", "LED BUILTIN", 2} // Led ON with signal HIGH
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
      if (msg[0] == '{')
        parseMessage(msg);
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
    // If this is a "writeOut" command, set the pin level to value
    String cmd;
    if (doc.getString("cmd", cmd)) {
      if (cmd == "writeOut") {
        double pin_val, level_val;
        if (doc.getNumber("pin", pin_val) && doc.getNumber("level", level_val)) {
          int pin = (int)pin_val;
          int level = (int)level_val;
          for (gpio_type &gpio : gpios) {
            if (gpio.pin == pin) {
              Serial.printf("Set pin %d to %d\n", pin, level);
              gpio.level = level;
              digitalWrite(pin, level);
              updateGpioList(nullptr);
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

void updateGpioList(AsyncWebServerRequest *request) {
  // Build JSON array manually since we need an array of objects
  String json = "[";
  
  bool first = true;
  for (gpio_type &gpio : gpios) {
    if (!first) json += ",";
    first = false;
    
    json += "{\"type\":\"";
    json += gpio.type;
    json += "\",\"pin\":";
    json += gpio.pin;
    json += ",\"label\":\"";
    json += gpio.label;
    json += "\",\"level\":";
    json += gpio.level;
    json += "}";
  }
  
  json += "]";
  
  // Update client via websocket
  server.broadcastWebSocket(json);

  if (request != nullptr)
    request->send(200, "text/plain", json);
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
  server.on("/getGpioList", HTTP_GET, updateGpioList);

  // Start server with custom websocket event handler
  server.begin(onWsEvent);
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
  server.handleClient();
  if (server.isAccessPointMode())
    server.updateDNS();

  // True on pin state change
  if (updateGpioState()) {
    updateGpioList(nullptr);   // Push new state to web clients via websocket
  }
}