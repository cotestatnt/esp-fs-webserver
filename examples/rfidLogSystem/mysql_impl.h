#include <MySQL.h>             //  https://github.com/cotestatnt/Arduino-MySQL
#include <WiFiClient.h>

extern String getSHA256(const char*);

// MySQL client class
WiFiClient client;
MySQL sql(&client, dbHost.c_str(), dbPort);

#define MAX_QUERY_LEN       512     // MAX query string length

static const char createUsersTable[] PROGMEM = R"string_literal(
CREATE TABLE %s (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(32) UNIQUE,
    password VARCHAR(128),
    name VARCHAR(64),
    email VARCHAR(64),
    tag_code BIGINT UNSIGNED UNIQUE,
    level INT
);


)string_literal";

static const char createLogTable[] PROGMEM = R"string_literal(
CREATE TABLE %s (
    id INT AUTO_INCREMENT PRIMARY KEY,
    epoch BIGINT,
    username VARCHAR(32),
    tag_code BIGINT UNSIGNED,
    reader INT UNSIGNED
);
)string_literal";


// Insert or update a device (if ble_id already defined keep it's actual value)
static const char newUpdateUser[] PROGMEM = R"string_literal(
INSERT INTO users (username, password, name, email, tag_code, level)
VALUES ('%s', '%s', '%s', '%s', %s, %s)
ON DUPLICATE KEY UPDATE
  username = VALUES(username),
  password = VALUES(password),
  name = VALUES(name),
  email = VALUES(email),
  tag_code = VALUES(tag_code),
  level = VALUES(level);
)string_literal";


// Establish connection with MySQL database according to the variables defined (/setup webpage)
bool connectToDatabase() {
  if (sql.connected()) {
    return true;
  }
  Serial.printf("\nConnecting to MySQL server %s on DataBase %s...\n", dbHost.c_str(), database.c_str());
  if (sql.connect(user.c_str(), password.c_str(), database.c_str())) {
    delay(200);
    return true;
  }
  Serial.println("Fails!");
  sql.disconnect();
  return false;
}


// Variadic function that will execute the query selected with passed parameters
bool queryExecute(DataQuery_t& data, const char* queryStr, ...) {
  if (connectToDatabase()) {
    char buf[MAX_QUERY_LEN];
    va_list args;
    va_start (args, queryStr);
    vsnprintf (buf, sizeof(buf), queryStr, args);
    va_end (args);

    // Execute the query
    Serial.printf("Execute SQL query: %s\n", buf);
    return sql.query(data, buf);
  }
  return false;
}

bool checkAndCreateTables() {
  // Create tables if not exists
  Serial.println("\nCreate table if not exists");
  DataQuery_t data;
  if (!queryExecute(data, createUsersTable, "users")) {
    if (strcmp(sql.getLastSQLSTATE(), "42S01") != 0)
      return false;
  }

  if (!queryExecute(data, createLogTable, "logs")) {
    if (strcmp(sql.getLastSQLSTATE(), "42S01") != 0)
      return false;
  }

  String hashed = getSHA256("admin");
  if (queryExecute(data, newUpdateUser, "admin", hashed.c_str(), "Update password!", "", "0", "5")) {
    Serial.println("admin@admin user created");
  }
  return true;
}

