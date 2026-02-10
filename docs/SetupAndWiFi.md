# Setup + WiFi (startWiFi / captive portal / config)

This library can:
- try to connect to a previously saved WiFi network (STA)
- if it fails, start an Access Point + captive portal and serve `/setup`
- persist **application options** into `config.json` on the filesystem  
  (WiFi credentials are now handled separately by `CredentialManager` and **never** stored in `config.json`)

## Typical flow

```cpp
if (!server.startWiFi(10000)) {
  server.startCaptivePortal("ESP_AP", "123456789", "/setup");
}
server.init(onWsEvent);
```

1. `startWiFi(timeout)` attempts to connect using saved credentials.
2. If it fails, `startCaptivePortal(ssid, pass, "/setup")` switches the ESP to AP mode and redirects requests to `/setup`.
3. On `/setup` the user selects the WiFi network (managed by `CredentialManager`) and any extra application options;  
  the library saves **only the application options** to `config.json` and stores WiFi credentials in encrypted form via `CredentialManager`.

It's possible also change the setting of IP address and mask for the captive portal passing a `WiFiConnectParams` structs to `startCaptivePortal()` method.
```cpp
if (!server.startWiFi(10000)) {
  Serial.println("\nWiFi not connected! Starting AP mode...");
  WiFiConnectParams params ("ESP_AP", "123456789");            
  params.config.local_ip = IPAddress(192, 168, 1, 1);
  params.config.gateway = IPAddress(192, 168, 1, 1);
  params.config.subnet = IPAddress(255, 255, 255, 0);    
  server.startCaptivePortal(params, "/setup");
}
```
## Application options on /setup

Example (see also `withWebSocket.ino`):

```cpp
server.addOptionBox("My Options");
server.addOption("LED Pin", ledPin);
server.addOption("Option 1", option1.c_str());
server.addOption("Option 2", option2);
```

Read your application options at boot (from `config.json`, WiFi excluded):

```cpp
uint32_t option2;
server.getOptionValue("Option 2", option2);
```

## Config file: read/write

- Full path: `server.getConfiFileName()`
- File access: `server.getConfigFile("r")` / `server.getConfigFile("w")`
- Reset config: `server.clearConfigFile()`

“Config saved” callback (useful when saving via `/edit` or upload):

```cpp
server.setConfigSavedCallback([](const char* filename){
  Serial.printf("Config saved: %s\n", filename);
});
```

## WiFi credentials storage (CredentialManager)

- WiFi SSID, password, DHCP/static IP and related data are **not** stored in `config.json`.
- They are managed and stored (encrypted) by the internal `CredentialManager`:
  - ESP32: NVS
  - ESP8266: filesystem (e.g. LittleFS)
- The `/setup` page talks directly with the WiFi APIs (`/wifi/credentials`, `/connect`, etc.),  
  so your sketch usually does **not** need to read or write WiFi data manually.

See also: `pwd_encrypt.md` for a deeper overview of `CredentialManager` and encrypted WiFi storage.

## Protect /setup with basic-auth

```cpp
server.setAuthentication("admin", "admin");
```

When set, the `/setup` page requires authentication.
