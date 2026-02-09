#include <Arduino.h>
#include <LittleFS.h>
#include <esp-fs-webserver.h>

// Configurator instance for testing
SetupConfigurator* configurator = nullptr;

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n\n");
    Serial.println("========================================");
    Serial.println("CONFIG UPGRADE TEST - v1 to v2.0");
    Serial.println("========================================");
    Serial.println();

    // Initialize filesystem
    if (!LittleFS.begin()) {
        Serial.println("❌ ERROR: Failed to mount LittleFS");
        return;
    }
    Serial.println("✓ LittleFS mounted");

    // Print original config
    // Serial.println("\n--- ORIGINAL CONFIG (v1.0) ---");
    // printFileContent("/config/config.json");

    // Upgrade with ConfigUpgrader
    Serial.println("\n--- UPGRADING CONFIG ---");
    ConfigUpgrader upgrader(&LittleFS, "/config/config.json");
    if (upgrader.upgrade("/config/config_v2.json")) {
        Serial.println("✓ Config upgraded in place");
    } else {
        Serial.println("❌ Error upgrading config");
    }

    // Print upgraded config
    Serial.println("\n--- UPGRADED CONFIG (v2.0) ---");
    printFileContent("/config/config_v2.json");

    Serial.println("\n--- UPGRADE TEST COMPLETED ---");
    Serial.println();
}

void loop() {
    delay(1000);
}


void printFileContent(const char* filepath) {
    if (!LittleFS.exists(filepath)) {
        Serial.print("❌ File not found: ");
        Serial.println(filepath);
        return;
    }

    File file = LittleFS.open(filepath, "r");
    if (!file) {
        Serial.print("❌ Failed to open: ");
        Serial.println(filepath);
        return;
    }

    Serial.println(filepath);
    
    // Simple line-by-line print (JSON already formatted by upgrader)
    while (file.available()) {
        String line = file.readStringUntil('\n');
        Serial.println(line);
    }
    
    file.close();
}
