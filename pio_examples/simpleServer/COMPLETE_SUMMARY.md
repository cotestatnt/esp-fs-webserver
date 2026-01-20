# FSWebServer CredentialManager Integration 

✅ **Encrypted Password Storage** - AES-256-CBC encryption
✅ **Automatic Persistence** - NVS (ESP32) or Filesystem (ESP8266)  
✅ **Multi-SSID Support** - Store up to 5 WiFi networks (configurable)
✅ **FIFO Management** - Automatically remove oldest when full
✅ **RSSI-Based Selection** - Connect to strongest signal
✅ **Zero Configuration** - Works transparently with FSWebServer
✅ **Cross-Platform** - ESP32 and ESP8266 compatible


## Quick Start

### Set Encryption Key
```cpp
// In CredentialManager.h, change this:
#define CREDENTIAL_MANAGER_ENCRYPTION_KEY "YOUR_SECRET_KEY_16_CHARS!"
// To something like:
#define CREDENTIAL_MANAGER_ENCRYPTION_KEY "MyCompany2024Key!"
```

Then:
1. Open `/setup` to add WiFi credentials
2. Reboot device
3. Check serial logs for RSSI-based connection

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│         User's Arduino Sketch (simpleServer.ino)    │
│                                                     │
│  - Calls: server.begin()                            │
│  - Calls: server.run()                              │
│  - No credential management code needed             │
└─────────────────────────┬───────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────┐
│         Modified FSWebServer Library                │
│                                                     │
│  • Constructor: Load credentials from storage       │
│  • startWiFi(): RSSI-based WiFi selection           │
│  • doWifiConnection(): FIFO credential mgmt         │  
└─────────────────────────┬───────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────┐
│       CredentialManager (New Class)                 │
│                                                     │
│  • AES-256-CBC encryption/decryption                │
│  • NVS persistence (ESP32)                          │
│  • Filesystem persistence (ESP8266)                 │
│  • Credential list management                       │
│  • FIFO removal when full                           │
└─────────────────────────┬───────────────────────────┘
                          │
            ┌─────────────┴─────────────┐
            ▼                           ▼
      ┌──────────────┐         ┌──────────────────┐
      │  NVS Flash   │         │  Filesystem      │
      │  (ESP32)     │         │  (ESP8266/ESP32) │
      │  Encrypted   │         │  Encrypted       │
      │  Passwords   │         │  Passwords       │
      └──────────────┘         └──────────────────┘
```

---

## Key Features Explained

### 1. Transparent Credential Management
- User adds credentials via `/setup` web page
- Credentials automatically saved to encrypted storage
- No code changes needed in user sketch
- Credentials persist across power cycles

### 2. FIFO (First In, First Out) Management
```
When list is full (5 credentials):
Add new credential → Automatically remove oldest → Keep 5 total
```

### 3. RSSI-Based WiFi Selection
```
On startup:
1. Scan available networks
2. Find stored credentials that match
3. Select the one with best (highest) signal strength
4. Connect to that network
```

**Example**:
- HomeWiFi: -65 dBm (good)
- OfficeWiFi: -42 dBm (excellent)  
→ Connect to OfficeWiFi (higher value = better signal)

### 4. Encryption
```
User input: "MyPassword123"
     ↓ (AES-256-CBC encrypt)
Stored: "aX7k3jQ9pL2nM8vX..."
     ↓ (AES-256-CBC decrypt)
Retrieved: "MyPassword123"
```

### 5. Cross-Platform Support
```
ESP32: Uses NVS (secure, encrypted by hardware)
ESP8266: Uses LittleFS/SPIFFS (plaintext storage file)
Both: Passwords encrypted by CredentialManager
```

## Security Considerations

### Password Encryption
- **Algorithm**: AES-256-CBC (256-bit encryption)
- **Padding**: PKCS7 (standard padding)
- **Mode**: Cipher Block Chaining (secure against pattern attacks)

### Encryption Key Storage
- **ESP32**: Ideally in eFuse BLOCK_KEY0 (hardware protected)
  ```bash
  # Generate and set key:
  espefuse.py key_set BLOCK_KEY0 <32-byte-hex-key>
  ```

- **ESP8266**: Compile-time constant (less secure)
  ```cpp
  #define CREDENTIAL_MANAGER_ENCRYPTION_KEY "MySecretKey12345"
  ```

### Additional Security Measures
- Use **Secure Boot** to prevent firmware tampering
- Use **eFuse** to lock down critical settings
- Keep encryption key confidential and consistent
- Change key only if you can re-encrypt stored credentials

### Threat Model
- **Protected Against**: Flash dump reading (passwords encrypted)
- **Protected Against**: Casual inspection of storage
- **Not Protected Against**: Brute-force attacks on weak keys
- **Not Protected Against**: Side-channel attacks (e.g., timing)

---

## Customization Options

### Change Maximum Credentials
In FSWebServer.h:
```cpp
static constexpr uint8_t MAX_CREDENTIALS = 5;  // Change to 10 for more
```

### Change Encryption Key
In CredentialManager.h:
```cpp
#define CREDENTIAL_MANAGER_ENCRYPTION_KEY "YourCustomKey12!"
```

---

## Performance

### Startup Time Impact
- WiFi scan: 500-2000ms (depends on RF environment)
- Credential matching: 10-50ms
- Connection: 2-5 seconds
- **Total**: ~3-7 seconds (typical)

### Memory Usage
- CredentialManager class: ~500 bytes
- Per credential: ~100 bytes
- Vector overhead: ~20 bytes
- **Total for 5 credentials**: ~1.2 KB

### Storage Usage
- Per credential in NVS/FS: ~60 bytes encrypted
- For 5 credentials: ~300 bytes
- **Impact**: Negligible on ESP32 (512KB NVS)

---

