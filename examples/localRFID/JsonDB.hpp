#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>

class TableManager {
public:
  TableManager(const char* filename) : filename(filename) {
    // Initialize as empty JSON array (will store our records)
    document.to<JsonArray>();
  }

  // Loads JSON data from LittleFS file into memory
  bool loadTable() {    
    File file = LittleFS.open(filename, FILE_READ);
    if (!file) {
      // File doesn't exist - initialize empty table (not an error)
      document.to<JsonArray>();
      return true;
    }

    document.clear();
    DeserializationError error = deserializeJson(document, file);
    file.close();

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return false;
    }
    return true;
  }

  // Checks if a record with same uniqueKey value already exists
  bool isDuplicate(JsonObject& newRecord, const char* uniqueKey) {
    JsonArray table = document.as<JsonArray>();
    if (newRecord[uniqueKey].is<const char*>()) {
      const char* idValue = newRecord[uniqueKey];
      for (JsonObject record : table) {
        if (record[uniqueKey].is<const char*>() && 
            strcmp(record[uniqueKey].as<const char*>(), idValue) == 0) {
          return true;
        }
      }
    }
    return false;
  }

  // Adds new record to the table with optional duplicate check
  bool addRecord(JsonObject newRecord, const char* uniqueKey = nullptr) {
    JsonArray table = document.as<JsonArray>();
    
    // Skip duplicate check if uniqueKey not provided
    if (uniqueKey && isDuplicate(newRecord, uniqueKey)) {
      return false;
    }
      
    if (table.add(newRecord))
      return saveTable();
    return false;
  }

  // Version that checks multiple unique keys
  bool addRecord(JsonObject newRecord, const char* uniqueKeys[], int numUniqueKeys) {
    JsonArray table = document.as<JsonArray>();

    // Check all specified unique keys for duplicates
    for (int i = 0; i < numUniqueKeys; i++) {          
      if (isDuplicate(newRecord, uniqueKeys[i]))
        return false;
    }

    if (table.add(newRecord))
      return saveTable();
    return false;
  }

  // Delete existing record matching key-value pair
  bool deleteRecord(const char* key, const char* value) {
    JsonArray table = document.as<JsonArray>();
    
    // Iterate through the array to find the matching record
    for (size_t i = 0; i < table.size(); i++) {
      JsonObject record = table[i].as<JsonObject>();
      if (record[key] == value) {          
        table.remove(i);
        return saveTable(); 
      }
    }
    return false; // Record not found
  }

  // Finds first record matching key-value pair
  JsonObject findRecord(const char* key, const char* value) {
    JsonArray table = document.as<JsonArray>();
    for (JsonObject record : table) {
      if (strcmp(record[key].as<const char*>(), value) == 0) {
        return record;        
      }
    }
    return JsonObject();
  }

  JsonArray getUsers() {
    loadTable();
    return document.as<JsonArray>();
  }

  // Persists current in-memory table to LittleFS
  bool saveTable() {
    File file = LittleFS.open(filename, FILE_WRITE);
    if (!file) return false;
    
    bool success = serializeJsonPretty(document, file) > 0;
    file.close();
    return success;
  }

  // Debug helper - prints current table to Serial
  void printTable() {
    serializeJsonPretty(document, Serial);
    Serial.println();
  }

private:
  const char* filename;  // LittleFS filename for persistence
  JsonDocument document; // In-memory JSON data storage
};
