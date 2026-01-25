#ifndef CREDENTIAL_MANAGER_H
#define CREDENTIAL_MANAGER_H

#include <Arduino.h>
#include <IPAddress.h>
#include <vector>
#include <cstring>

#if defined(ESP32)
    #include <mbedtls/aes.h>
    #include "esp_efuse.h"
    #include "nvs_flash.h"
#elif defined(ESP8266)
    // Use local compatibility shim for AES on ESP8266 (BearSSL-backed)
    #include "compat/mbedtls_aes.h"
    #include "LittleFS.h"
#endif

/**
 * @brief Default encryption key (32 bytes for AES-256)
 * 
 * IMPORTANT: Modify this with your own 32-byte key!
 * ESP32: This is used as fallback if BLOCK_KEY0 is not programmed
 * ESP8266: This is always used (no eFuse available)
 * 
 * Example generation:
 * $ python3 -c "import os; key = os.urandom(32); print('0x' + ',0x'.join(f'{b:02x}' for b in key))"
 */
#ifndef CREDENTIAL_MANAGER_ENCRYPTION_KEY
#define CREDENTIAL_MANAGER_ENCRYPTION_KEY \
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, \
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99
#endif

/**
 * @class CredentialManager
 * @brief Manages WiFi credentials with transparent AES-256 encryption
 * 
 * Features:
 * - Automatic encryption with AES-256-CBC
 * - ESP32: Support for BLOCK_KEY0 (eFuse) with secure fallback
 * - ESP8266: User-defined encryption key (no eFuse available)
 * - Persistent storage in NVS (ESP32) or SPIFFS (ESP8266)
 * - Multiple SSID/password management
 * - Transparent: works seamlessly across platforms
 * - Passwords always encrypted in memory and storage
 */

struct WiFiCredential {
    char ssid[33];                  // Max 32 char SSID + null
    uint8_t password_encrypted[64]; // Encrypted password
    uint16_t password_len;          // Length of encrypted data
    IPAddress gateway;              // Optional static IP config
    IPAddress subnet;               // Optional static IP config    
    IPAddress local_ip;             // Optional static IP config
};

class CredentialManager {
public:
    friend class FSWebServer;
    static const uint8_t MAX_CREDENTIALS = 5;
    static const uint8_t ENCRYPTION_KEY_SIZE = 32; // 256-bit AES
    static const uint16_t AES_BLOCK_SIZE = 16;

    /**
     * @brief Constructor - initializes the manager
     */
    CredentialManager();

    /**
     * @brief Destructor - frees resources
     */
    ~CredentialManager();

    /**
     * @brief Initialize encryption key from eFuse (ESP32) or predefined constant (ESP8266)
     * @return true if successful, false on error
     */
    bool begin();

    /**
     * @brief Return encryption security status
     * @return true if encryption is properly configured
     */
    bool isSecure() const { return m_efuse_initialized; }

    /**
     * @brief Return status description
     * @return String with security status and platform info
     */
    String getStatus() const;

    /**
     * @brief Add a WiFi credential (automatically encrypted)
     * @param credential WiFiCredential struct containing SSID and optional IP config
     * @param plaintext_password Plaintext password (will be encrypted internally)
     * @return true if successfully added
     */
    bool addCredential(const WiFiCredential& credential, const char* plaintext_password);

    /**
     * @brief Update a WiFi credential (automatically encrypted)
     * @param credential WiFiCredential struct with IP config and SSID to identify
     * @param plaintext_password Plaintext password (will be encrypted internally)
     * @return true if successfully updated
     */
    bool updateCredential(const WiFiCredential& credential, const char* plaintext_password);

    /**
     * @brief Get number of stored credentials
     * @return Credential count
     */
    uint8_t getCredentialCount() const { return m_credentials.size(); }

    /**
     * @brief Get SSID of a credential
     * @param index Credential index
     * @return SSID (nullptr if index invalid)
     */
    const char* getSSID(uint8_t index) const;

    /**
     * @brief Get decrypted password of a credential
     * @param index Credential index
     * @return Password in plaintext (automatic removal from memory after use)
     */
    String getPassword(uint8_t index);

    String getPassword(const char* ssid) {
        for (size_t i = 0; i < m_credentials.size(); i++) {
            if (strcmp(m_credentials[i].ssid, ssid) == 0) {
                return getPassword(i);
            }
        }
        return "";
    }


    /**
     * @brief Set static IP configuration for a credential
     * @param index Credential index
     * @param ip Local IP address (0.0.0.0 for DHCP)
     * @param gateway Gateway IP address
     * @param subnet Subnet mask
     * @return true if successfully set
     */
    bool setIPConfiguration(uint8_t index, IPAddress ip, IPAddress gateway, IPAddress subnet);

    /**
     * @brief Get static IP configuration for a credential
     * @param index Credential index
     * @param ip Output: Local IP address
     * @param gateway Output: Gateway IP address
     * @param subnet Output: Subnet mask
     * @return true if valid configuration exists
     */
    bool getIPConfiguration(uint8_t index, IPAddress& ip, IPAddress& gateway, IPAddress& subnet) const;

    /**
     * @brief Clear all credentials from memory
     */
    void clearAll();

    /**
     * @brief Remove a single credential by index
     * @param index Credential index
     * @return true if removed
     */
    bool removeCredential(uint8_t index);

    /**
     * @brief Remove a single credential by SSID
     * @param ssid Network SSID
     * @return true if removed
     */
    bool removeCredential(const char* ssid);

    #if defined(ESP8266)
    /**
     * @brief Set filesystem reference for persistence (ESP8266/ESP32)
     * @param fs Pointer to FS object (LittleFS or SPIFFS)
     * @note Required for saveToFS() and loadFromFS() to work on ESP8266
     */
    void setFilesystem(fs::FS* fs);

    
    /**
     * @brief Save credentials to filesystem (ESP8266/SPIFFS)
     * @param filepath Path to save file
     * @return true if successful
     */
    bool saveToFS(const char* filepath = "/credentials.bin");

    /**
     * @brief Load credentials from filesystem (ESP8266/SPIFFS)
     * @param filepath Path to load file from
     * @return true if at least one credential loaded
     */
    bool loadFromFS(const char* filepath = "/credentials.bin");
    #endif

    #if defined(ESP32)
    /**
     * @brief Save credentials to NVS (ESP32 only)
     * @param nvs_namespace NVS namespace to use
     * @return true if successful
     */
    bool saveToNVS(const char* nvs_namespace = "wifi_creds");

    /**
     * @brief Load credentials from NVS (ESP32 only)
     * @param nvs_namespace NVS namespace to use
     * @return true if at least one credential loaded
     */
    bool loadFromNVS(const char* nvs_namespace = "wifi_creds");
    #endif

    /**
     * @brief Get BLOCK_KEY0 status details
     * @return Technical status details
     */
    String getDebugInfo() const;

    std::vector<WiFiCredential>* getCredentials() {
        return &m_credentials;
    }

    bool checkSSIDExists(const char* ssid) const {
        for (const auto& cred : m_credentials) {
            if (strcmp(cred.ssid, ssid) == 0) {
                return true;
            }
        }
        return false;
    }

protected:
    std::vector<WiFiCredential> m_credentials;
    uint8_t m_encryption_key[ENCRYPTION_KEY_SIZE];
    bool m_efuse_initialized;

    #if defined(ESP8266)
    fs::FS* m_filesystem;
    #endif

    /**
     * @brief Load key from eFuse (ESP32) or predefined constant (ESP8266)
     */
    void initializeEncryptionKey();

    /**
     * @brief Encrypt password with AES-256-CBC
     * @param plaintext Password in plaintext
     * @param ciphertext Output encrypted buffer
     * @param cipher_len Output: length of encrypted data
     * @return true if successful
     */
    bool encryptPassword(const char* plaintext, uint8_t* ciphertext, uint16_t& cipher_len);

    /**
     * @brief Decrypt password with AES-256-CBC
     * @param ciphertext Encrypted data
     * @param cipher_len Length of encrypted data
     * @param plaintext Output password buffer
     * @param max_len Maximum buffer size
     * @return true if successful
     */
    bool decryptPassword(const uint8_t* ciphertext, uint16_t cipher_len,
                        char* plaintext, uint16_t max_len);

    /**
     * @brief Apply PKCS7 padding
     * @param data Input buffer
     * @param data_len Length of original data
     * @param padded Output buffer with padding
     * @param padded_len Output: total length with padding
     */
    void applyPKCS7Padding(const uint8_t* data, uint16_t data_len,
                           uint8_t* padded, uint16_t& padded_len);

    /**
     * @brief Remove PKCS7 padding
     * @param data Input buffer
     * @param data_len Buffer length
     * @return Length of data without padding (0 if error)
     */
    uint16_t removePKCS7Padding(uint8_t* data, uint16_t data_len);
};

#endif // CREDENTIAL_MANAGER_H
