#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <FSWebServer.h>  // https://github.com/cotestatnt/esp-fs-webserver
#include <MFRC522v2.h>         // https://github.com/OSSLibraries/Arduino_MFRC522v2
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include <time.h>

#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"  // Timezone definition to get properly time from NTP server

/*
* To customize the appearance of the web pages, upload the login and rfid files 
* (without extensions) to the microcontroller's flash memory using built-in functionality (/setup webpage). 
* Set the following #define to false for send your pages instead of those embedded in the firmware. 
* You can find both files in the "/data" folder.
*/
#define USE_EMBEDDED_HTM true

#if USE_EMBEDDED_HTM
#include "data/html_login.h"
#include "data/html_rfid.h"
#endif

#define PIN_MISO    15
#define PIN_MOSI    16
#define PIN_SCLK    17
#define PIN_CS      18

uint32_t chipId = 0;                   // Unique ID of the ESP chip
bool addLogRecord = true;              // Flag to enable/disable log recording

MFRC522DriverPinSimple ss_pin(PIN_CS); // Configurable, see typical pin layout above.
MFRC522DriverSPI driver{ss_pin};       // Create SPI driver.
MFRC522 mfrc522{driver};               // Create MFRC522 instance.
struct tm ntp;                         // Time structure holding current time from NTP

#include "JsonDB.hpp"
#include "webserver.hpp"

TableManager usersTable("/users.json");

struct User_t{
  const char* username;
  const char* password;
  const char* name;
  const char* email;
  const char* tag;
  uint8_t level;
} ;

// Some test users
User_t users[] = {
  {"admin", "admin", "DB adminitrator", "", "", 5},
  {"user1", "", "John Doe", "jhon.doe@email.com", "12345", 1},
  {"user2", "", "Mark twain", "mark65@email.com", "67890", 1}
};

const char* uniqueKeys[] = {"tag", "username"};

//////////////////////////// Append a row to csv file ///////////////////////////////////
bool appendRow(const char* username, uint64_t tag) {
  getLocalTime(&ntp, 10);

  char filename[32];
  snprintf(filename, sizeof(filename),"/logs/%04d_%02d_%02d.csv", ntp.tm_year + 1900, ntp.tm_mon + 1, ntp.tm_mday);  

  File file = LittleFS.open("/logs");
  if (!file) {    
    LittleFS.mkdir("/logs");
    Serial.println("Created /logs directory");
  }
  
  if (LittleFS.exists(filename)) {
    file = LittleFS.open(filename, "a");   // Append to existing file
  }
  else {
    file = LittleFS.open(filename, "w");   // Create a new file    
    file.println("reader, username, tag, timestamp");
    Serial.printf("Created file %s\n", filename);
  }

  if (file) {
    char timestamp[25];
    strftime(timestamp, sizeof(timestamp), "%c", &ntp);
    char row[64];
    snprintf(row, sizeof(row), "%d, %s, %llu, %s", chipId, username, tag, timestamp);        
    Serial.println(row);
    file.println(row);
    file.close();
    return true;
  }
  return false;
}

void setup() {
  SPI.begin(PIN_SCLK, PIN_MISO, PIN_MOSI, PIN_CS);
  Serial.begin(115200);
  Serial.println("\n\n\n\nStarting ESP32 RFID gateway...");

  mfrc522.PCD_Init();                                       // Init MFRC522 board.
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);	  // Show details of PCD - MFRC522 Card Reader details.
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
  
  if (!LittleFS.begin()) {         
    Serial.println("ERROR on mounting filesystem. It will be reformatted!");
    LittleFS.format();
    ESP.restart();
  }

  // LittleFS.remove("/users.json");

  // Load the users table
  usersTable.loadTable();

  for (User_t user: users){
    JsonDocument userDoc;
    userDoc["username"] = user.username;
    userDoc["password"] = getSHA256(user.password);
    userDoc["name"] = user.name;
    userDoc["email"] = user.email;
    userDoc["tag"] = user.tag;
    userDoc["level"] = user.level;

    if (usersTable.addRecord(userDoc.as<JsonObject>(), uniqueKeys, 2)) 
      Serial.println("Record added to table");
    else
      Serial.println("Error or record alreay present.");
  }

  // Start webserver
  startWebServer();
  // Set NTP servers
  #ifdef ESP8266
  configTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
  #elif defined(ESP32)
  configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
  #endif
  // Wait for NTP sync (with timeout)
  getLocalTime(&ntp, 5000);

  #if defined(ESP32)
  for(byte i=0; i<17; i=i+8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  #elif defined(ESP8266)
  chipId = ESP.getChipId();
  #endif
  Serial.print("ESP Chip ID: ");
  Serial.println(chipId);
}

void loop() {
  server.handleClient();
  if (server.isAccessPointMode())
    server.updateDNS();

  delay(10);

	if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    static uint32_t readTime = millis();
    static uint64_t oldCode = 0;

    // The UID of an RFID tag can be up to 64bit long
    uint64_t tagCode = 0;   

    // tagCode is swapped, but it doesn't matter We need only it's a unique number
    for(byte i = 0; i < mfrc522.uid.size; i++) {
      tagCode |= mfrc522.uid.uidByte[i] << (8*i);
    }

    if ((tagCode != oldCode) || (millis() - readTime > 5000)) {
      readTime = millis();
      oldCode = tagCode;

      // Serial.printf("\nTag code: %llu\n", tagCode);
      if (addLogRecord) {                
        JsonObject user = usersTable.findRecord("tag", String(tagCode).c_str());
        if (user) {        
          const char* username = user["username"].as<const char*>();
          appendRow(username, tagCode);
        }
        else {
          appendRow("not allowed", tagCode);
        }
      }
    }
	}
}


