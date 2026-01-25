#include "CredentialManager.h"
#include "SerialLog.h"


CredentialManager::CredentialManager() : m_efuse_initialized(false) {
  memset(m_encryption_key, 0, ENCRYPTION_KEY_SIZE);
#if defined(ESP8266)
  m_filesystem = nullptr;
#endif
}

CredentialManager::~CredentialManager() {
  // Clean key from memory
  memset(m_encryption_key, 0, ENCRYPTION_KEY_SIZE);
  clearAll();
}

bool CredentialManager::begin() {
  initializeEncryptionKey();
  return true;
}

void CredentialManager::initializeEncryptionKey() {
#if defined(ESP32)
  esp_err_t err = esp_efuse_read_block(EFUSE_BLK_KEY0, m_encryption_key, 0,
                                       ENCRYPTION_KEY_SIZE * 8);

  if (err == ESP_OK) {
    // Check if truly programmed (not all 0xFF)
    bool all_ff = true;
    for (int i = 0; i < ENCRYPTION_KEY_SIZE; i++) {
      if (m_encryption_key[i] != 0xFF) {
        all_ff = false;
        break;
      }
    }

    if (!all_ff) {
      m_efuse_initialized = true;
      log_info("BLOCK_KEY0 initialized from eFuse - SECURE mode");
    } else {
      log_info("BLOCK_KEY0 not programmed - using fallback (NOT SECURE)");
      m_efuse_initialized = false;
      // Use fallback key (derived from MAC address or constant)
      // This obfuscates but is not real security
      memset(m_encryption_key, 0xAA, ENCRYPTION_KEY_SIZE);
    }
  } else {
    log_error("Failed to read BLOCK_KEY0: %s", esp_err_to_name(err));
    // Fallback to predefined key
    memset(m_encryption_key, 0xAA, ENCRYPTION_KEY_SIZE);
  }
#else
  // For ESP8266 or other chips, use fallback
  memset(m_encryption_key, 0xAA, ENCRYPTION_KEY_SIZE);
#endif
}

String CredentialManager::getStatus() const {
  if (m_efuse_initialized) {
    return "SECURE (BLOCK_KEY0 programmed)";
  } else {
    return "INSECURE (BLOCK_KEY0 not programmed - fallback key used)";
  }
}

bool CredentialManager::addCredential(const WiFiCredential& credential, const char* plaintext_password) {
  if (strlen(credential.ssid) == 0) {
    log_error("Invalid SSID");
    return false;
  }
  if (!plaintext_password || strlen(plaintext_password) == 0) {
    log_error("Invalid password");
    return false;
  }

  if (m_credentials.size() >= MAX_CREDENTIALS) {
    log_error("Maximum credentials reached (%d)", MAX_CREDENTIALS);
    return false;
  }

  if (strlen(plaintext_password) > 63) {
    log_error("Password too long (max 63 characters)");
    return false;
  }

  // Create credential copy with encrypted password
  WiFiCredential cred = credential;

  // Encrypt password
  uint16_t encrypted_len = 0;
  if (!encryptPassword(plaintext_password, cred.password_encrypted, encrypted_len)) {
    log_error("Failed to encrypt password");
    return false;
  }

  cred.password_len = encrypted_len;

  m_credentials.push_back(cred);
  log_debug("Credential added: %s.", cred.ssid);

  return true;
}

bool CredentialManager::removeCredential(uint8_t index) {
  if (index >= m_credentials.size()) {
    log_error("Invalid credential index %d", index);
    return false;
  }

  log_debug("Removing credential: %s", m_credentials[index].ssid);
  memset(m_credentials[index].password_encrypted, 0, sizeof(m_credentials[index].password_encrypted));
  memset(m_credentials[index].ssid, 0, sizeof(m_credentials[index].ssid));
  m_credentials.erase(m_credentials.begin() + index);
  return true;
}

bool CredentialManager::removeCredential(const char* ssid) {
  if (!ssid) {
    return false;
  }

  for (size_t i = 0; i < m_credentials.size(); i++) {
    if (strcmp(m_credentials[i].ssid, ssid) == 0) {
      return removeCredential(static_cast<uint8_t>(i));
    }
  }

  log_error("Credential not found: %s", ssid);
  return false;
}

bool CredentialManager::updateCredential(const WiFiCredential& credential, const char* plaintext_password) {
  uint8_t index = 255;
  for (size_t i = 0; i < m_credentials.size(); i++) {
    if (strcmp(m_credentials[i].ssid, credential.ssid) == 0) {
      index = i;
      break;
    }
  }
  
  if (index >= m_credentials.size()) {
    log_error("Credential not found: %s", credential.ssid);
    return false;
  }

  if (!plaintext_password || strlen(plaintext_password) == 0) {
    log_error("Invalid password");
    return false;
  }

  if (strlen(plaintext_password) > 63) {
    log_error("Password too long (max 63 characters)");
    return false;
  }

  WiFiCredential &cred = m_credentials[index];

  // Encrypt new password
  uint16_t encrypted_len = 0;
  if (!encryptPassword(plaintext_password, cred.password_encrypted, encrypted_len)) {
    log_error("Failed to encrypt password");
    return false;
  }

  cred.password_len = encrypted_len;

  // Update IP configuration from credential parameter
  cred.gateway = credential.gateway;
  cred.subnet = credential.subnet;
  cred.local_ip = credential.local_ip;
  
  log_debug("Credential updated: %s.", credential.ssid);
  return true;
}

const char *CredentialManager::getSSID(uint8_t index) const {
  if (index >= m_credentials.size()) {
    return nullptr;
  }
  return m_credentials[index].ssid;
}

String CredentialManager::getPassword(uint8_t index) {
  if (index >= m_credentials.size()) {
    return "";
  }

  const WiFiCredential &cred = m_credentials[index];
  char plaintext[65] = {0};

  if (decryptPassword(cred.password_encrypted, cred.password_len, plaintext,
                      64)) {
    String result = plaintext;
    // Clean password from memory
    memset(plaintext, 0, 65);
    return result;
  }

  return "";
}

bool CredentialManager::setIPConfiguration(uint8_t index, IPAddress ip, IPAddress gateway, IPAddress subnet) {
  if (index >= m_credentials.size()) {
    log_error("Invalid credential index %d", index);
    return false;
  }

  m_credentials[index].local_ip = ip;
  m_credentials[index].gateway = gateway;
  m_credentials[index].subnet = subnet;

  log_debug("IP configuration updated for credential %d: IP=%s, GW=%s, SN=%s", 
            index, ip.toString().c_str(), gateway.toString().c_str(), subnet.toString().c_str());
  return true;
}

bool CredentialManager::getIPConfiguration(uint8_t index, IPAddress& ip, IPAddress& gateway, IPAddress& subnet) const {
  if (index >= m_credentials.size()) {
    log_error("Invalid credential index %d", index);
    return false;
  }

  ip = m_credentials[index].local_ip;
  gateway = m_credentials[index].gateway;
  subnet = m_credentials[index].subnet;

  return true;
}

void CredentialManager::clearAll() {
  // Clean each credential
  for (auto &cred : m_credentials) {
    memset(cred.password_encrypted, 0, sizeof(cred.password_encrypted));
    memset(cred.ssid, 0, sizeof(cred.ssid));
  }
  m_credentials.clear();
  #if defined(ESP8266)
    saveToFS();
  #elif defined(ESP32)
    saveToNVS();
  #endif
  log_debug("All credentials cleared");
}

bool CredentialManager::encryptPassword(const char *plaintext, uint8_t *ciphertext, uint16_t &cipher_len) {
  if (!plaintext || !ciphertext) {
    return false;
  }

  size_t plaintext_len = strlen(plaintext);
  if (plaintext_len > 63) {
    log_error("Password too long");
    return false;
  }

  // Apply PKCS7 padding
  uint8_t *padded = new uint8_t[64];
  uint16_t padded_len = 0;
  applyPKCS7Padding((uint8_t *)plaintext, plaintext_len, padded, padded_len);

  // AES-256-CBC encryption
  mbedtls_aes_context aes_ctx;
  mbedtls_aes_init(&aes_ctx);

  int ret = mbedtls_aes_setkey_enc(&aes_ctx, m_encryption_key, 256);
  if (ret != 0) {
    log_error("AES setkey failed: %d", ret);
    mbedtls_aes_free(&aes_ctx);
    delete[] padded;
    return false;
  }

  // Fixed IV (deterministic, not randomized)
  // In case of randomized IV, save it in the first block
  uint8_t iv[AES_BLOCK_SIZE] = {0};

  ret = mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_ENCRYPT, padded_len, iv,
                              padded, ciphertext);

  mbedtls_aes_free(&aes_ctx);
  delete[] padded;

  if (ret != 0) {
    log_error("AES encryption failed: %d", ret);
    return false;
  }

  cipher_len = padded_len;
  return true;
}

bool CredentialManager::decryptPassword(const uint8_t *ciphertext, uint16_t cipher_len, char *plaintext, uint16_t max_len) {
  if (!ciphertext || !plaintext || cipher_len == 0 ||
      cipher_len % AES_BLOCK_SIZE != 0) {
    log_error("Invalid decryption parameters");
    return false;
  }

  if (cipher_len > max_len) {
    log_error("Buffer too small for decryption");
    return false;
  }

  // AES-256-CBC decryption
  mbedtls_aes_context aes_ctx;
  mbedtls_aes_init(&aes_ctx);

  int ret = mbedtls_aes_setkey_dec(&aes_ctx, m_encryption_key, 256);
  if (ret != 0) {
    log_error("AES setkey failed: %d", ret);
    mbedtls_aes_free(&aes_ctx);
    return false;
  }

  uint8_t iv[AES_BLOCK_SIZE] = {0};
  uint8_t *decrypted = new uint8_t[cipher_len];

  ret = mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_DECRYPT, cipher_len, iv,
                              ciphertext, decrypted);

  mbedtls_aes_free(&aes_ctx);

  if (ret != 0) {
    log_error("AES decryption failed: %d", ret);
    delete[] decrypted;
    return false;
  }

  // Remove PKCS7 padding
  uint16_t plaintext_len = removePKCS7Padding(decrypted, cipher_len);
  if (plaintext_len == 0) {
    log_error("Invalid padding in decrypted data");
    delete[] decrypted;
    return false;
  }

  if (plaintext_len >= max_len) {
    log_error("Plaintext too long");
    delete[] decrypted;
    return false;
  }

  memcpy(plaintext, decrypted, plaintext_len);
  plaintext[plaintext_len] = '\0';

  // Clean sensitive data from memory
  memset(decrypted, 0, cipher_len);
  delete[] decrypted;

  return true;
}

void CredentialManager::applyPKCS7Padding(const uint8_t *data, uint16_t data_len, uint8_t *padded, uint16_t &padded_len) {
  // Calculate length with padding (multiple of 16)
  uint16_t total_len = ((data_len / AES_BLOCK_SIZE) + 1) * AES_BLOCK_SIZE;
  uint8_t pad_value = total_len - data_len;

  memcpy(padded, data, data_len);
  for (int i = data_len; i < total_len; i++) {
    padded[i] = pad_value;
  }

  padded_len = total_len;
}

uint16_t CredentialManager::removePKCS7Padding(uint8_t *data, uint16_t data_len) {
  if (data_len < AES_BLOCK_SIZE || data_len % AES_BLOCK_SIZE != 0) {
    return 0;
  }

  uint8_t pad_value = data[data_len - 1];

  // Validate padding
  if (pad_value > AES_BLOCK_SIZE || pad_value == 0) {
    return 0;
  }

  // Verify that all padding bytes have the correct value
  for (int i = data_len - pad_value; i < data_len; i++) {
    if (data[i] != pad_value) {
      return 0;
    }
  }

  return data_len - pad_value;
}

#if defined(ESP8266)
void CredentialManager::setFilesystem(fs::FS* fs) {
  m_filesystem = fs;
}

bool CredentialManager::saveToFS(const char* filepath) {
  if (!m_filesystem) {
    log_error("Filesystem not set");
    return false;
  }

  File file = m_filesystem->open(filepath, "w");
  if (!file) {
    log_error("Failed to open %s for write", filepath);
    return false;
  }

  uint8_t count = m_credentials.size();
  if (count > MAX_CREDENTIALS) {
    count = MAX_CREDENTIALS;
  }

  file.write(&count, 1);

  for (uint8_t i = 0; i < count; i++) {
    const WiFiCredential &cred = m_credentials[i];

    file.write((const uint8_t*)cred.ssid, sizeof(cred.ssid));
    file.write((const uint8_t*)&cred.password_len, sizeof(cred.password_len));
    file.write((const uint8_t*)cred.password_encrypted, sizeof(cred.password_encrypted));

    uint32_t gw_addr = cred.gateway;
    uint32_t sn_addr = cred.subnet;
    uint32_t ip_addr = cred.local_ip;
    file.write((const uint8_t*)&gw_addr, sizeof(gw_addr));
    file.write((const uint8_t*)&sn_addr, sizeof(sn_addr));
    file.write((const uint8_t*)&ip_addr, sizeof(ip_addr));
  }

  file.close();
  log_info("Credentials saved to FS (%d entries)", count);
  return true;
}

bool CredentialManager::loadFromFS(const char* filepath) {
  if (!m_filesystem) {
    log_error("Filesystem not set");
    return false;
  }

  if (!m_filesystem->exists(filepath)) {
    log_info("No credentials file found: %s", filepath);
    return false;
  }

  File file = m_filesystem->open(filepath, "r");
  if (!file) {
    log_error("Failed to open %s for read", filepath);
    return false;
  }

  m_credentials.clear();

  uint8_t count = 0;
  if (file.read(&count, 1) != 1 || count == 0) {
    file.close();
    log_debug("No credentials in FS");
    return false;
  }

  if (count > MAX_CREDENTIALS) {
    count = MAX_CREDENTIALS;
  }

  for (uint8_t i = 0; i < count; i++) {
    WiFiCredential cred{};

    if (file.read((uint8_t*)cred.ssid, sizeof(cred.ssid)) != sizeof(cred.ssid)) break;
    if (file.read((uint8_t*)&cred.password_len, sizeof(cred.password_len)) != sizeof(cred.password_len)) break;
    if (cred.password_len == 0 || cred.password_len > sizeof(cred.password_encrypted)) break;
    if (file.read((uint8_t*)cred.password_encrypted, sizeof(cred.password_encrypted)) != sizeof(cred.password_encrypted)) break;

    uint32_t gw_addr = 0;
    uint32_t sn_addr = 0;
    uint32_t ip_addr = 0;
    if (file.read((uint8_t*)&gw_addr, sizeof(gw_addr)) != sizeof(gw_addr)) break;
    if (file.read((uint8_t*)&sn_addr, sizeof(sn_addr)) != sizeof(sn_addr)) break;
    if (file.read((uint8_t*)&ip_addr, sizeof(ip_addr)) != sizeof(ip_addr)) break;

    cred.gateway = IPAddress(gw_addr);
    cred.subnet = IPAddress(sn_addr);
    cred.local_ip = IPAddress(ip_addr);

    m_credentials.push_back(cred);
  }

  file.close();

  if (m_credentials.size() > 0) {
    log_info("Loaded %d credentials from FS", m_credentials.size());
    return true;
  }

  return false;
}
#endif


#if defined(ESP32)
bool CredentialManager::saveToNVS(const char *nvs_namespace) {

  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open(nvs_namespace, NVS_READWRITE, &nvs_handle);

  if (err != ESP_OK) {
    log_error("Failed to open NVS: %s", esp_err_to_name(err));
    return false;
  }

  // Save number of credentials
  err = nvs_set_u8(nvs_handle, "count", m_credentials.size());
  if (err != ESP_OK) {
    log_error("Failed to save credentials count");
    nvs_close(nvs_handle);
    return false;
  }

  // Save each credential
  for (size_t i = 0; i < m_credentials.size(); i++) {
    const WiFiCredential &cred = m_credentials[i];

    // SSID key
    String key_ssid = "ssid" + String(i);
    err = nvs_set_str(nvs_handle, key_ssid.c_str(), cred.ssid);
    if (err != ESP_OK) {
      log_error("Failed to save SSID %d", i);
      nvs_close(nvs_handle);
      return false;
    }

    // Encrypted password key (as blob)
    String key_pass = "pass" + String(i);
    err = nvs_set_blob(nvs_handle, key_pass.c_str(), cred.password_encrypted,
                       cred.password_len);
    if (err != ESP_OK) {
      log_error("Failed to save password %d", i);
      nvs_close(nvs_handle);
      return false;
    }

    // Password length
    String key_len = "len" + String(i);
    err = nvs_set_u16(nvs_handle, key_len.c_str(), cred.password_len);
    if (err != ESP_OK) {
      log_error("Failed to save password length %d", i);
      nvs_close(nvs_handle);
      return false;
    }

    // Static IP Configuration - Gateway
    String key_gw = "gw" + String(i);
    uint32_t gw_addr = cred.gateway;
    err = nvs_set_u32(nvs_handle, key_gw.c_str(), gw_addr);
    if (err != ESP_OK) {
      log_error("Failed to save gateway %d", i);
      nvs_close(nvs_handle);
      return false;
    }

    // Static IP Configuration - Subnet
    String key_sn = "sn" + String(i);
    uint32_t sn_addr = cred.subnet;
    err = nvs_set_u32(nvs_handle, key_sn.c_str(), sn_addr);
    if (err != ESP_OK) {
      log_error("Failed to save subnet %d", i);
      nvs_close(nvs_handle);
      return false;
    }

    // Static IP Configuration - Local IP
    String key_ip = "ip" + String(i);
    uint32_t ip_addr = cred.local_ip;
    err = nvs_set_u32(nvs_handle, key_ip.c_str(), ip_addr);
    if (err != ESP_OK) {
      log_error("Failed to save local IP %d", i);
      nvs_close(nvs_handle);
      return false;
    }
  }

  err = nvs_commit(nvs_handle);
  nvs_close(nvs_handle);

  if (err == ESP_OK) {
    log_info("Credentials saved to NVS (%d entries)", m_credentials.size());
    return true;
  } else {
    log_error("Failed to commit NVS");
    return false;
  }
}

bool CredentialManager::loadFromNVS(const char *nvs_namespace) {
  m_credentials.clear();
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open(nvs_namespace, NVS_READONLY, &nvs_handle);

  if (err != ESP_OK) {
    log_info("No NVS data found for namespace: %s", nvs_namespace);
    return false;
  }

  // Read number of credentials
  uint8_t count = 0;
  err = nvs_get_u8(nvs_handle, "count", &count);
  if (err != ESP_OK || count == 0) {
    log_debug("No credentials in NVS");
    nvs_close(nvs_handle);
    return false;
  }

  if (count > MAX_CREDENTIALS) {
    count = MAX_CREDENTIALS;
  }

  // Read each credential
  for (int i = 0; i < count; i++) {
    WiFiCredential cred;
    memset(&cred, 0, sizeof(cred));

    // Read SSID
    String key_ssid = "ssid" + String(i);
    size_t ssid_len = 33;
    err = nvs_get_str(nvs_handle, key_ssid.c_str(), cred.ssid, &ssid_len);
    if (err != ESP_OK) {
      log_error("Failed to load SSID %d", i);
      continue;
    }

    // Read password length
    String key_len = "len" + String(i);
    uint16_t pass_len = 0;
    err = nvs_get_u16(nvs_handle, key_len.c_str(), &pass_len);
    if (err != ESP_OK || pass_len == 0 || pass_len > 64) {
      log_error("Invalid password length for credential %d", i);
      continue;
    }

    // Read encrypted password
    String key_pass = "pass" + String(i);
    size_t blob_len = pass_len;
    err = nvs_get_blob(nvs_handle, key_pass.c_str(), cred.password_encrypted,
                       &blob_len);
    if (err != ESP_OK) {
      log_error("Failed to load password %d", i);
      continue;
    }

    cred.password_len = pass_len;

    // Read Static IP Configuration - Gateway
    String key_gw = "gw" + String(i);
    uint32_t gw_addr = 0;
    err = nvs_get_u32(nvs_handle, key_gw.c_str(), &gw_addr);
    if (err == ESP_OK) {
      cred.gateway = IPAddress(gw_addr);
    } else {
      cred.gateway = IPAddress(0, 0, 0, 0);
    }

    // Read Static IP Configuration - Subnet
    String key_sn = "sn" + String(i);
    uint32_t sn_addr = 0;
    err = nvs_get_u32(nvs_handle, key_sn.c_str(), &sn_addr);
    if (err == ESP_OK) {
      cred.subnet = IPAddress(sn_addr);
    } else {
      cred.subnet = IPAddress(0, 0, 0, 0);
    }

    // Read Static IP Configuration - Local IP
    String key_ip = "ip" + String(i);
    uint32_t ip_addr = 0;
    err = nvs_get_u32(nvs_handle, key_ip.c_str(), &ip_addr);
    if (err == ESP_OK) {
      cred.local_ip = IPAddress(ip_addr);
    } else {
      cred.local_ip = IPAddress(0, 0, 0, 0);
    }

    m_credentials.push_back(cred);
  }

  nvs_close(nvs_handle);

  if (m_credentials.size() > 0) {
    log_info("Loaded %d credentials from NVS", m_credentials.size());
    return true;
  }

  return false;
}
#endif

String CredentialManager::getDebugInfo() const {
  String info = "\n=== Credential Manager Debug Info ===\n";

  info += "Encryption Status: " + getStatus() + "\n";
  info += "Credentials Count: " + String(m_credentials.size()) + "/" +
          String(MAX_CREDENTIALS) + "\n";

#if defined(ESP32)
  info += "BLOCK_KEY0 Status: ";

  // Read BLOCK_KEY0 status
  uint8_t efuse_key[32];
  esp_err_t err = esp_efuse_read_block(EFUSE_BLK_KEY0, efuse_key, 0, 256);

  if (err == ESP_OK) {
    bool all_ff = true;
    bool all_00 = true;
    for (int i = 0; i < 32; i++) {
      if (efuse_key[i] != 0xFF)
        all_ff = false;
      if (efuse_key[i] != 0x00)
        all_00 = false;
    }

    if (all_ff) {
      info += "NOT PROGRAMMED (all 0xFF)\n";
    } else if (all_00) {
      info += "INVALID (all 0x00)\n";
    } else {
      info += "PROGRAMMED (bytes present)\n";
    }
  } else {
    info += "ERROR: " + String(esp_err_to_name(err)) + "\n";
  }
#else
  info += "BLOCK_KEY0 Status: N/A (not ESP32)\n";
#endif

  info += "\nCredentials:\n";
  for (size_t i = 0; i < m_credentials.size(); i++) {
    info += "  [" + String(i) + "] SSID: " + String(m_credentials[i].ssid);
    
    // Show IP configuration if not all zeros
    if (m_credentials[i].local_ip != IPAddress(0, 0, 0, 0)) {
      info += " | IP: " + m_credentials[i].local_ip.toString();
      info += " | GW: " + m_credentials[i].gateway.toString();
      info += " | SN: " + m_credentials[i].subnet.toString();
    } else {
      info += " | IP: DHCP";
    }
    
    info += " | Encrypted len: " + String(m_credentials[i].password_len) + "\n";
  }

  info += "=====================================\n";
  return info;
}
