#include <LittleFS.h>
#include <MySQL.h>             //  https://github.com/cotestatnt/Arduino-MySQL
#include <esp-fs-webserver.h>  //  https://github.com/cotestatnt/esp-fs-webserver
#include "mbedtls/md.h"

#include "html_flash_files.h"
extern MySQL sql;
extern bool queryExecute(DataQuery_t&, const char*, ...);

// Webserver class
FSWebServer myWebServer(LittleFS, 80, "esp32rfid");

int getUserLevel(const String& user, const String&  hash) {
  DataQuery_t data;
  if (queryExecute(data, "SELECT password, level FROM users WHERE username = '%s';", user)) {
    sql.printResult(data, Serial);
    if (hash.equals(data.getRowValue(0, "password")))
      return atoi(data.getRowValue(0, "level"));
  }
  return 0;
}


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


void handleGetLogs() {
  String filter =  myWebServer.arg("filter");
  String SQL = "SELECT * FROM logs ";
  if (filter.length()) {
    SQL += filter;
  }
  SQL += " ORDER BY epoch DESC LIMIT 30;";
  Serial.println(SQL);
  delay(100);

  DataQuery_t data;
  if (queryExecute(data, SQL.c_str())) {
    sql.printResult(data, Serial);

    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();;
    for (Record_t &row : data.records) {
      JsonObject user = array.add<JsonObject>();
      user["id"] = row.record[0];
      user["epochTime"] = row.record[1];
      user["username"] = row.record[2];
      user["tagCode"] = row.record[3];
      user["readerID"] = row.record[4];
    }
    String json;
    serializeJsonPretty(doc, json);
    myWebServer.send(200, "application/json", json);
    return;
  }
  myWebServer.send(500, "text/plain", sql.getLastError());
}

void handleGetUsers() {
  DataQuery_t data;
  if (queryExecute(data, "SELECT id, username, name, email, tag_code, level FROM users")) {
    sql.printResult(data, Serial);

    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();
    for (Record_t &row : data.records) {
      JsonObject user = array.add<JsonObject>();
      user["id"] = row.record[0];
      user["username"] = row.record[1];
      user["name"] = row.record[2];
      user["email"] = row.record[3];
      user["tagCode"] = row.record[4];
      user["level"] = row.record[5];
    }
    String json;
    serializeJsonPretty(doc, json);
    myWebServer.send(200, "application/json", json);
    return;
  }
  myWebServer.send(500, "text/plain", sql.getLastError());
}

void handleNewUser() {
  String user = myWebServer.arg("username");
  String name = myWebServer.arg("name");
  String email = myWebServer.arg("email");
  String tagCode = myWebServer.arg("tagCode");
  String level = myWebServer.arg("level");
  String hashedPassword = getSHA256(myWebServer.arg("password").c_str());
  
  DataQuery_t data;
  if (queryExecute(data, newUpdateUser,
    myWebServer.arg("username").c_str(), hashedPassword.c_str(), myWebServer.arg("name").c_str(),
    myWebServer.arg("email").c_str(), myWebServer.arg("tagCode").c_str(), myWebServer.arg("level").c_str()))
  {
    myWebServer.send(200, "text/plain", "OK");
    return;
  }
  myWebServer.send(500, "text/plain", sql.getLastError());
}

void handleRemoveUser() {
  DataQuery_t data;
  if (queryExecute(data, "DELETE FROM users WHERE username = '%s';", myWebServer.arg("username").c_str())) {
    myWebServer.send(200, "text/plain", "OK");
    return;
  }
  myWebServer.send(500, "text/plain", sql.getLastError());
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

      String result = "{\"tagCode\": ";
      result += tagCode;
      result += "}";

      Serial.print("\nTag code: ");
      Serial.println(tagCode);
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
    String cookie = "username=" ;
    cookie += myWebServer.arg("username");
    cookie += ",";  cookie += level;  cookie += "; Path=/";
    myWebServer.sendHeader("Set-Cookie", cookie);
    myWebServer.send_P(200, "text/html", (const char*)index_htm, sizeof(index_htm));
  } 
  else {
    myWebServer.send(401, "text/plain", "Wrong password");
  }
}

////////////////////  Load application options from filesystem  ////////////////////
bool loadOptions() {
  if (LittleFS.exists(myWebServer.getConfigFilepath())) {
    myWebServer.getOptionValue(MY_SQL_HOST, dbHost);
    myWebServer.getOptionValue(MY_SQL_PORT, dbPort);
    myWebServer.getOptionValue(MY_SQL_DB, database);
    myWebServer.getOptionValue(MY_SQL_USER, user);
    myWebServer.getOptionValue(MY_SQL_PASS, password);
    Serial.printf(MY_SQL_HOST ": %s\n", dbHost.c_str());
    Serial.printf(MY_SQL_PORT ": %d\n", dbPort);
    Serial.printf(MY_SQL_DB ": %s\n", database.c_str());
    Serial.printf(MY_SQL_USER ": %s\n", user.c_str());
    Serial.printf(MY_SQL_PASS ": %s\n", password.c_str());
    return true;
  }
  else
    Serial.printf("File \"%s\" not exist\n", myWebServer.getConfigFilepath());
  return false;
}

////////////////////////////////  Filesystem  /////////////////////////////////////////

// Configure and start webserver
void startWebServer() {
  // FILESYSTEM INIT
  if (!LittleFS.begin()) {
    Serial.println("ERROR on mounting filesystem. It will be formmatted!");
    LittleFS.format();
    ESP.restart();
  }

  // Load configuration (if not present, default will be created when webserver will start)
  Serial.println("Load application otions:");
  if (!loadOptions())
    Serial.println("Error!! Options NOT loaded!");
  Serial.println();

  // Set IP address when in AP mode
  myWebServer.setIPaddressAP(IPAddress(192,168,4,1));

  // Try to connect to stored SSID, start AP if fails after timeout of 15 seconds
  myWebServer.setAP("", "");
  IPAddress serverIP = myWebServer.startWiFi(15000, true);

  // Configure /setup page and start Web Server
  myWebServer.addOptionBox("MySQL setup");
  myWebServer.addOption(MY_SQL_HOST, dbHost);
  myWebServer.addOption(MY_SQL_PORT, dbPort);
  myWebServer.addOption(MY_SQL_DB, database);
  myWebServer.addOption(MY_SQL_USER, user);
  myWebServer.addOption(MY_SQL_PASS, password);

  // Add endpoints request handlers
  myWebServer.on("/logs", HTTP_ANY, handleGetLogs);
  myWebServer.on("/users", HTTP_GET, handleGetUsers);
  myWebServer.on("/addUser", HTTP_POST, handleNewUser);
  myWebServer.on("/deleUser", HTTP_POST, handleRemoveUser);
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
  myWebServer.on("/login", HTTP_ANY, [](){
    myWebServer.send_P(200, "text/html", (const char*)login_htm, sizeof(login_htm));
  });
  myWebServer.on("/rfid", HTTP_POST, handleCheckHash);
  /*
  * If client calculated password SHA256 hash string match with the user DB stored
  * we can serve the /rfid web page (from flash literal string, same as login page)
  */
  myWebServer.on("/rfid", HTTP_GET, handleMainPage);

  // To enable add/edit/delete buttons, user must be admin (level >= 5)
  myWebServer.on("/userLevel", HTTP_GET, []{
    DataQuery_t data;
    if (queryExecute(data, "SELECT password, level FROM users WHERE username = '%s';", myWebServer.arg("username"))) {
      String cookie = "user_level=" + String(data.getRowValue(0, "level")) + "; Path=/";
      myWebServer.sendHeader("Set-Cookie", cookie);
      myWebServer.send(200, "text/plain", "OK");
    }
  });
  
  // Enable ACE FS file web editor and add FS info callback fucntion
  // myWebServer.enableFsCodeEditor([](fsInfo_t* fsInfo) {
  //   fsInfo->totalBytes = LittleFS.totalBytes();
  //   fsInfo->usedBytes = LittleFS.usedBytes();
  //   fsInfo->fsName = "LittleFS";
  // });

  // set /setup and /edit page authentication
  myWebServer.setAuthentication("admin", "admin");

  // Start the webserver
  myWebServer.begin();
  Serial.print("\n\nESP Web Server started on IP Address: ");
  Serial.println(serverIP);
  Serial.println(
    "Open /setup page to configure optional parameters.\n"
    "Open /edit page to view, edit or upload example or your custom webserver source files."
  );
}