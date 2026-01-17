# Setup + WiFi (startWiFi / captive portal / config)

This library can:
- try to connect to a previously saved WiFi network (STA)
- if it fails, start an Access Point + captive portal and serve `/setup`
- persist configuration into `config.json` on the filesystem

## Typical flow

```cpp
if (!server.startWiFi(10000)) {
  server.startCaptivePortal("ESP_AP", "123456789", "/setup");
}
server.init(onWsEvent);
```

1. `startWiFi(timeout)` attempts to connect using saved credentials.
2. If it fails, `startCaptivePortal(ssid, pass, "/setup")` switches the ESP to AP mode and redirects requests to `/setup`.
3. On `/setup` the user selects the network and options; the library saves them to the FS.

## Application options on /setup

Example (see also `withWebSocket.ino`):

```cpp
server.addOptionBox("My Options");
server.addOption("LED Pin", ledPin);
server.addOption("Option 1", option1.c_str());
server.addOption("Option 2", option2);
```

Read values at boot (from `config.json`):

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

## Protect /setup with basic-auth

```cpp
server.setAuthentication("admin", "admin");
```

When set, the `/setup` page requires authentication.
