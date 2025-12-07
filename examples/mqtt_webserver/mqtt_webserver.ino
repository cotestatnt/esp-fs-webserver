/*
 Basic ESP MQTT example:
 It connects to an MQTT server then:
  - publishes "hello world" to the topic "output" every two seconds
  - subscribes to the topic "input", printing out any (string) messages it receives.
  - If the first character of the topic "input" is an 1, switch ON the ESP Led, else switch it off
  It will reconnect to the server using a NON blocking reconnect function. 
*/

#include <WiFi.h>
#include <PubSubClient.h>      // https://github.com/knolleary/pubsubclient/
#include <esp-fs-webserver.h>  // https://github.com/cotestatnt/esp-fs-webserver

#include <FS.h>
#include <LittleFS.h>

FSWebServer myWebServer(LittleFS, 80);

// Update these with values suitable for your network.
const char* mqtt_server = "broker.mqtt-dashboard.com";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Let'use our unique clientID and topics (tied to cliendId in setup())
char clientId[16];    
char inTopic[24];
char outTopic[24];

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
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character (LED on with LOW signal on most boards)
  digitalWrite(BUILTIN_LED, (char)payload[0] == '1' ? LOW : HIGH);  
}

///////////////////////////  MQTT reconnect function  ///////////////////////////////////
void mqttReconnect() {
  static uint32_t lastConnectionTime = 5000;     
  if (millis() - lastConnectionTime < 5000)         // Wait 5 seconds before retrying
    return;
  lastConnectionTime = millis();

  Serial.print("Attempting MQTT connection...");
  if (mqttClient.connect(clientId)) {       // Attempt to connect
    Serial.println("connected");

    String payload = "Hello World from ";
    payload += clientId;
    mqttClient.publish(outTopic, payload.c_str());   // Once connected, publish an announcement...
    mqttClient.subscribe(inTopic);                   // ... and resubscribe
  } 
  else {
    Serial.print("failed, rc=");
    Serial.print(mqttClient.state());
    Serial.println(" try again in 5 seconds");
  }
}



void setup() {
  pinMode(BUILTIN_LED, OUTPUT);  // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  
  // LittleFS filesystem init
  startFilesystem();

  // Try to connect to flash stored SSID, start AP if fails after timeout
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

  // Set MQTT server and callback function
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(mqttCallback);

  // Create a unique mqttClient ID and in/out topics
  snprintf(clientId, sizeof(clientId), "ESP-%llX", ESP.getEfuseMac());
  snprintf(inTopic, sizeof(inTopic), "%s/input", clientId);
  snprintf(outTopic, sizeof(outTopic), "%s/output", clientId);

  Serial.print("MQTT CLiend ID: ");
  Serial.println(clientId);
  Serial.print("Publish output topic: ");
  Serial.println(outTopic);
  Serial.print("Subscribe input topic: ");
  Serial.println(inTopic);
}

void loop() {

  // Handle MQTT client
  if (!mqttClient.connected()) {
    mqttReconnect();
  }
  mqttClient.loop();

  // Publish a new message every 5 seconds
  static uint32_t lastMsgTime = millis();
  static uint16_t value = 0;
  if (millis() - lastMsgTime > 5000) {
    lastMsgTime = millis();

    String payload = "Hello World from ";
    payload += clientId;
    payload += " #";
    payload += ++value;

    Serial.print("Publish message: ");
    Serial.println(payload);
    mqttClient.publish(outTopic, payload.c_str());
  }

  // Handle webserver request (just /setup and /edit in this example)
  myWebServer.run();
}
