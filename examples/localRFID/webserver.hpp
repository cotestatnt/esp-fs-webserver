#pragma once
#include <Arduino.h>

#include <LittleFS.h>
#include <FSWebServer.h>  // https://github.com/cotestatnt/esp-fs-webserver
#include "mbedtls/md.h"


// Webserver class
FSWebServer myWebServer(80, LittleFS, "esp32rfid");

extern TableManager usersTable;
extern const char* uniqueKeys[];


String getSHA256(const char* payload) {
  String hashed = "";
  byte shaResult[32];
  mbedtls_md_context_t ctx;
  const size_t payloadLength = strlen(payload);         
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, (const unsigned char *) payload, payloadLength);
  mbedtls_md_finish(&ctx, shaResult);
  mbedtls_md_free(&ctx);
  for(int i= 0; i< sizeof(shaResult); i++){
    char str[3];
    sprintf(str, "%02x", (int)shaResult[i]);
    hashed += str;
  }
  return hashed;
}

int getUserLevel(const String& username, const String&  hash) {
  JsonObject user = usersTable.findRecord("username", username.c_str());
  if (user) {
    if (hash.equals(user["password"].as<const char*>())) {
      return user["level"].as<int>();
    }
  }
  return 0;
}

void handleGetUsers() {
  JsonArray users = usersTable.getUsers();
  String json;
  serializeJsonPretty(users, json);
  myWebServer.send(200, "application/json", json);
}

void handleNewUser() {
  String username = myWebServer.arg("username");
  String name = myWebServer.arg("name");
  String email = myWebServer.arg("email");
  String tag = myWebServer.arg("tag");
  String level = myWebServer.arg("level");
  String hashedPassword = getSHA256(myWebServer.arg("password").c_str());
  bool update = myWebServer.arg("type").equals("Update");

  JsonDocument newUser;
  newUser["username"] = username;
  newUser["password"] = hashedPassword;
  newUser["name"] = name;
  newUser["email"] = email;
  newUser["tag"] = tag;
  newUser["level"] = level;

  usersTable.loadTable();

  if (update){
    if (!usersTable.deleteRecord("username", username.c_str()))
      myWebServer.send(500, "text/plain", "Error");
  }
  
  if (usersTable.addRecord(newUser.as<JsonObject>(),uniqueKeys, 2))  {
    myWebServer.send(200, "text/plain", "OK");
    return;
  } 
  myWebServer.send(500, "text/plain", "Error");
}

void handleRemoveUser() {
  String username = myWebServer.arg("username");
  if (usersTable.deleteRecord("username", username.c_str()))
    myWebServer.send(200, "text/plain", "OK");
  else
    myWebServer.send(500, "text/plain", "Error");
}

void handleGetCode() {
  uint32_t timeout = millis();

  while (true) {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      uint64_t tagCode = 0;

      // tagCode is swapped, but it doesn't matter We need only it's a unique number
      for(byte i = 0; i < mfrc522.uid.size; i++) {
        tagCode |= mfrc522.uid.uidByte[i] << (8*i);
      }
      
      // With 8 byte TAG code, the result integer could be too large since JavaScript 
      // uses 64-bit floating-point numbers (IEEE 754), which have a maximum precision of 2^53 - 1 
      String result = "{\"tag\": \"";
      result += String(tagCode);  
      result += "\"}";

      Serial.printf("Tag code: 0x%llX", tagCode);
      myWebServer.send(200, "application/json", result);
      addLogRecord = true;
      return;
    }

    if (millis() - timeout > 5000) {
      myWebServer.send(500, "application/json", "{\"error\": \"timeout\"}");
      addLogRecord = true;
      return;
    }
  }
}

// This handler will be called from login page to check password
void handleCheckHash() {

  // Even if user con login, only user with level >= 5 can edit users table
  if (getUserLevel(myWebServer.arg("username"), myWebServer.arg("hash"))) {
    myWebServer.send(200, "text/plain", "OK");
  }
  else {
    myWebServer.send(401, "text/plain", "Wrong password");
  }
}


// This handler will be called from login page on login succesfull
void handleMainPage() {
  // Check again user and password to avoid direct page loading
  int level = getUserLevel(myWebServer.arg("username"), myWebServer.arg("hash"));
  if (level) {
    // Even if any user con login succesfully, only user with level >= 5 can edit users table
    // Username and user level is set here using cookie.
    String cookie = "username=" ;
    cookie += myWebServer.arg("username");
    cookie += ",";  cookie += level;  cookie += "; Path=/";
#if USE_EMBEDDED_HTM
    myWebServer.sendHeader("Content-Encoding", "gzip");
    myWebServer.sendHeader("Set-Cookie", cookie);
    myWebServer.send_P(200, "text/html", (const char*)rfid, sizeof(rfid));
#else
    myWebServer.sendHeader("Set-Cookie", cookie);
    myWebServer.send(200, "text/html", "");  // Send file from filesystem if not embedded
#endif
  } 
  else {
    myWebServer.send(401, "text/plain", "Wrong password");
  }
}


// Configure and start webserver
bool startWebServer(bool clear = false) {
  bool connected = false;
  // FILESYSTEM INIT
  if (!LittleFS.begin()) {
    Serial.println("ERROR on mounting filesystem. It will be formmatted!");
    LittleFS.format();
    ESP.restart();
  }

  if (clear) {
    LittleFS.remove(myWebServer.getConfiFileName());
  }

  // Try to connect to WiFi (will start AP if not connected after timeout)
  if (!myWebServer.startWiFi(15000)) {
    Serial.println("\nWiFi not connected! Starting AP mode...");
    myWebServer.startCaptivePortal("ESP32_RFID", "", "/setup");
  }
  else 
    connected = true;
    
  // Add endpoints request handlers
  myWebServer.on("/users", HTTP_GET, handleGetUsers);
  myWebServer.on("/addUser", HTTP_POST, handleNewUser);
  myWebServer.on("/delUser", HTTP_POST, handleRemoveUser);
  myWebServer.on("/getCode", HTTP_GET, handleGetCode);
  myWebServer.on("/waitCode", HTTP_GET, [](){
    addLogRecord = false; 
    myWebServer.send(200, "text/plain", "OK");
  });

  /* 
  * To avoid ugly and basic login prompt avalaible with "stardard" DIGEST_AUTH
  * let's use a custom login web page (from flash literal string). This web page
  * will send a POST request to /rfid enpoint passing username and password SHA256 hash
  */
  myWebServer.on("/", HTTP_ANY, [](){    
    // Redirect to login page
    myWebServer.sendHeader("Location", "/login");
    myWebServer.send(302, "text/plain", "");
  });

  #if USE_EMBEDDED_HTM
  myWebServer.on("/login", HTTP_ANY, [](){
    myWebServer.sendHeader("Content-Encoding", "gzip");
    myWebServer.send_P(200, "text/html", (const char*)rfid, sizeof(rfid));
  });
  #endif
  
  /*
  * If client calculated password SHA256 hash string match with the user stored,
  * we can serve the /rfid web page (from flash literal string, same as login page)
  */
  myWebServer.on("/rfid", HTTP_POST, handleCheckHash);

  /*
  * Although it is not the conventional way, handle this page with a PUT request
  * otherwise it will be impossible to edit file if needed.
  * The embedded /edit webpage uses the GET method to retrieve the file content.
  */
  myWebServer.on("/rfid", HTTP_PUT, handleMainPage);

  // Enable ACE FS file web editor and add FS info callback function
  myWebServer.enableFsCodeEditor();

  // Start the webserver
  myWebServer.begin();
  Serial.print("\n\nESP Web Server started on IP Address: ");
  Serial.println(myWebServer.getServerIP());
  Serial.println(
    "Open /setup page to configure optional parameters.\n"
    "Open /edit page to view, edit or upload example or your custom webserver source files."
  );

  return connected;
}