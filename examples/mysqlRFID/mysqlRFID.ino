#include <FS.h>
#include <LittleFS.h>

#include <ArduinoJson.h>       // https://github.com/bblanchon/ArduinoJson
#include <MySQL.h>             // https://github.com/cotestatnt/Arduino-MySQL
#include <FSWebServer.h>  // https://github.com/cotestatnt/esp-fs-webserver
#include <MFRC522v2.h>         // https://github.com/OSSLibraries/Arduino_MFRC522v2
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>

#define PIN_MISO    15
#define PIN_MOSI    16
#define PIN_SCLK    17
#define PIN_CS      18

// This set of variables can be updated using webpage http://<esp-ip-address>/setup
String user = "xxxxxxxxxxx";                   // MySQL user login username
String password = "xxxxxxxxxxxx";              // MySQL user login password
String dbHost = "192.168.1.1";                 // MySQL hostname/URL
String database = "xxxxxxxxxxx";               // Database name
uint16_t dbPort = 3306;                        // MySQL host port

// Var labels (in /setup webpage)
#define MY_SQL_HOST "MySQL Hostname"
#define MY_SQL_PORT "MySQL Port"
#define MY_SQL_DB   "MySQL Database name"
#define MY_SQL_USER "MySQL Username"
#define MY_SQL_PASS "MySQL Password"
#define RFID_READER_ID "RFID Reader ID"

MFRC522DriverPinSimple ss_pin(PIN_CS); // Configurable, see typical pin layout above.
MFRC522DriverSPI driver{ss_pin};       // Create SPI driver.
MFRC522 mfrc522{driver};               // Create MFRC522 instance.

uint32_t chipId = 0;
bool addLogRecord = true;

#include "mysql_impl.h"
#include "webserver_impl.h"

void setup() {
  SPI.begin(PIN_SCLK, PIN_MISO, PIN_MOSI, PIN_CS);
  Serial.begin(115200);
  Serial.println("\n\n\n\nStarting ESP32 RFID gateway...");
  
  mfrc522.PCD_Init();  // Init MFRC522 board.
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);	// Show details of PCD - MFRC522 Card Reader details.
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
  
  /* Init and start configuration webserver */
  if (startWebServer()) {
  
    /* Init and start MySQL task */
    if (connectToDatabase()) {
      /*
      * Check if working table exists and create if not exists.
      * If not already present, also a default admin user will be created (admin, admin);
      */ 
      if (!checkAndCreateTables()) {
        Serial.println("Error! Tables not created properly");
      }
    }
    else {
      Serial.println("\nDatabase connection failed.\nCheck your connection");
    }
  }
  
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
      oldCode = tagCode;
      readTime = millis();
      Serial.printf("\nTag code: %llu\n", tagCode);
  
      if (addLogRecord) {
        DataQuery_t data;
        if (queryExecute(data, "SELECT username, level FROM users WHERE tag_code = %llu;", tagCode)) {
          sql.printResult(data, Serial);
          String SQL = "INSERT INTO logs (epoch, username, tag_code, reader) VALUES (UNIX_TIMESTAMP(), '";
          SQL += data.getRowValue(0, "username");  SQL += "', ";
          SQL += tagCode; SQL += ", ";
          SQL += chipId; SQL += ");";
  
          if (queryExecute(data, SQL.c_str())) {
            Serial.printf("\"%s\" registered on reader %lu\n", data.getRowValue(0, "username"), chipId);
          }
        }
      }
    }
  }
}

