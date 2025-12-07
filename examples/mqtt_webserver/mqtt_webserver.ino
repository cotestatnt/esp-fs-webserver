/*
 Basic ESP32 MQTT example:
 It connects to an MQTT server then:
  - publishes "hello world" to the topic "output" every two seconds
  - subscribes to the topic "input", printing out any (string) messages it receives.
  - If the first character of the topic "input" is an 1, switch ON the ESP Led, else switch it off
  It will reconnect to the server using a NON blocking reconnect function. 
*/
#include <FS.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include <PubSubClient.h>      // https://github.com/knolleary/pubsubclient/
#include <esp-fs-webserver.h>  // https://github.com/cotestatnt/esp-fs-webserver

#ifndef BUILTIN_LED
#define BUILTIN_LED 2  // Pin number for the built-in LED on ESP32 boards
#endif

FSWebServer myWebServer(LittleFS, 80);

// Update these with values suitable for your network.
const char* mqtt_server = "broker.mqtt-dashboard.com";

////////////////////////////////  Filesystem  /////////////////////////////////////////
void startFilesystem() {
  if ( !LittleFS.begin()) {
    Serial.println("ERROR on mounting filesystem. It will be formmatted!");
    LittleFS.format();
    ESP.restart();
  }
  myWebServer.printFileList(LittleFS, Serial, "/", 2);
}

///////////////////////////  MQTT callback function  ///////////////////////////////////
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("Message arrived [%s] ", topic);
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character (LED on with LOW signal on most boards)
  digitalWrite(BUILTIN_LED, (char)payload[0] == '1' ? LOW : HIGH);
}


// Declare task handle
TaskHandle_t mqttTaskHandle = NULL;

void mqttTask(void *parameter) {
  // Create WiFiClient and PubSubClient locally in the task
  WiFiClient espClient;
  PubSubClient mqttClient(espClient);

  uint32_t lastMsgTime = millis();
  uint32_t lastConnectionTime = 0;
  uint16_t value = 0;

  // Set MQTT server and callback function
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(mqttCallback);

  // Create a unique mqttClient ID and in/out topics
  char clientId[16];
  char inTopic[24];
  char outTopic[24];
  snprintf(clientId, sizeof(clientId), "ESP-%llX", ESP.getEfuseMac());
  snprintf(inTopic, sizeof(inTopic), "%s/input", clientId);
  snprintf(outTopic, sizeof(outTopic), "%s/output", clientId);

  Serial.print("MQTT CLiend ID: "); Serial.println(clientId);
  Serial.print("Publish output topic: "); Serial.println(outTopic);
  Serial.print("Subscribe input topic: "); Serial.println(inTopic);

  // Task infinite loop
  for (;;) { 
    // Yield to other tasks
    vTaskDelay(100 / portTICK_PERIOD_MS);  

    // Debug: print stack info every 30 seconds
    static uint32_t lastStackCheck = 0;
    if (millis() - lastStackCheck > 30000) {
      lastStackCheck = millis();
      UBaseType_t stackWatermark = uxTaskGetStackHighWaterMark(NULL);
      Serial.printf("[MQTT Task] Stack high water mark: %u bytes Free\n",  stackWatermark * 4);
    }

    // Handle MQTT connection
    if (!mqttClient.connected()) {
      // Try to reconnect every 5 seconds if WiFi is connected
      if (WiFi.status() == WL_CONNECTED && millis() - lastConnectionTime > 5000) {
        lastConnectionTime = millis();

        Serial.print("Attempting MQTT connection...");
        if (mqttClient.connect(clientId)) {
          Serial.println("connected");
          char payload[64];
          snprintf(payload, sizeof(payload), "Hello World from %s", clientId);
          mqttClient.publish(outTopic, payload);
          mqttClient.subscribe(inTopic);
        } 
        else {
          Serial.printf("failed, rc=%d\n", mqttClient.state());
        }
      }
    } 
    else {
      // Client connected, handle MQTT messages
      mqttClient.loop();

      // Publish a new message every 5 seconds
      if (millis() - lastMsgTime > 5000) {
        lastMsgTime = millis();

        char payload[64];
        snprintf(payload, sizeof(payload), "Hello World from %s #%d", clientId, ++value);
        mqttClient.publish(outTopic, payload);
        Serial.print("Publish message: ");
        Serial.println(payload);
      }
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);  // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\nESP32 MQTT & WebServer Example");

  // LittleFS filesystem init
  startFilesystem();

  // Try to connect to stored SSID, start AP if fails after timeout
  myWebServer.setAP("ESP32", "123456789");
  IPAddress myIP = myWebServer.startWiFi(15000);
  Serial.println("\n");

  // Enable ACE FS file web editor and add FS info callback function
  myWebServer.enableFsCodeEditor([](fsInfo_t* fsInfo) {
    fsInfo->fsName = "LittleFS";
    fsInfo->totalBytes = LittleFS.totalBytes();
    fsInfo->usedBytes = LittleFS.usedBytes();
  });

  // Start webserver
  myWebServer.begin();
  Serial.print(F("ESP Web Server started on IP Address: "));
  Serial.println(myIP);
  Serial.println(F("Open /setup page to configure optional parameters"));
  Serial.println(F("Open /edit page to view and edit files"));

  // Create and start MQTT task
  xTaskCreate(
    mqttTask,         // Task function
    "mqttTask",       // Task name
    10000,            // Stack size (bytes)
    NULL,             // Parameters
    10,               // Priority
    &mqttTaskHandle   // Task handle
  );
  Serial.printf("[DEBUG] MQTT Task created with stack size: 10000 bytes (~2500 words)\n");
  
}

void loop() {
  // Nothing to do here for MQTT, all handled in mqttTask
  delay(10);
  
  myWebServer.run();  // Handle web server requests
}
