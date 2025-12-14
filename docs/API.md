# AsyncFsWebServer – API (overview)

This page summarizes the main methods exposed by `AsyncFsWebServer` and when to use them.

> Note: some APIs are available only when the related features are enabled via macros (e.g. `ESP_FS_WS_SETUP`, `ESP_FS_WS_EDIT`).

## Constructor

```cpp
AsyncFsWebServer(uint16_t port, fs::FS &fs, const char* hostname = "");
```

- `port`: HTTP port (typically `80`)
- `fs`: filesystem (`LittleFS`, `SPIFFS`, `FFat`, …)
- `hostname`: optional (also used for mDNS)

## Server start

```cpp
bool init(AwsEventHandler wsHandle = nullptr);
```

- Registers built-in handlers (setup/edit if enabled), static file serving, and notFound.
- If `wsHandle != nullptr`, creates (or reuses) a websocket on `/ws`.

## Runtime info

```cpp
IPAddress getServerIP();
bool isAccessPointMode() const;
```

## Authentication (/setup page)

```cpp
void setAuthentication(const char* user, const char* pswd);
```

When set, the `/setup` page requires basic-auth.

## Filesystem listing

```cpp
void printFileList(fs::FS &fs, const char * dirname, uint8_t levels);
void printFileList(fs::FS &fs, const char * dirname, uint8_t levels, Print& out);
```

- The `Print& out` overload lets you send output to streams other than `Serial`.

Example:

```cpp
server.printFileList(FILESYSTEM, "/", 1);           // default -> Serial
server.printFileList(FILESYSTEM, "/", 1, Serial);  // explicit
```

## WiFi + captive portal

```cpp
bool startWiFi(uint32_t timeout, CallbackF fn = nullptr);
bool startCaptivePortal(const char* ssid, const char* pass, const char* redirectTargetURL);
```

- `startWiFi()` tries to connect using already-saved credentials.
- If it fails, you typically start `startCaptivePortal()` and use `/setup` to configure.

## WebSocket (runtime)

```cpp
AsyncWebSocket* getWebSocket();
void enableWebSocket(const char* path, AwsEventHandler handler);
void wsBroadcast(const char * buffer);
void wsBroadcastBinary(uint8_t * message, size_t len);
```

## Setup page (only if `ESP_FS_WS_SETUP`)

Config file and callback:

```cpp
File getConfigFile(const char* mode);
const char* getConfiFileName();
bool clearConfigFile();
void setConfigSavedCallback(ConfigSavedCallbackF callback);
```

Options and setup UI:

```cpp
void setSetupPageTitle(const char* title);
void addOptionBox(const char* title);

template <typename T>
void addOption(const char *lbl, T val, bool hidden=false, double min=MIN_F, double max=MAX_F, double st=1.0);

template <typename T>
bool getOptionValue(const char *lbl, T &var);

template <typename T>
bool saveOptionValue(const char *lbl, T val);
```

Dropdown/Slider:

```cpp
using DropdownList = AsyncFSWebServer::DropdownList;
using Slider = AsyncFSWebServer::Slider;

void addDropdownList(DropdownList &def);
void addSlider(Slider &def);
bool getDropdownSelection(DropdownList &def);
bool getSliderValue(Slider &def);
```

## Web file editor (only if `ESP_FS_WS_EDIT`)

```cpp
void enableFsCodeEditor(FsInfoCallbackF fsCallback = nullptr);
void setFsInfoCallback(FsInfoCallbackF fsCallback);
```

See also: `SetupAndWiFi.md`, `FileEditorAndFS.md`, `WebSocket.md`.
