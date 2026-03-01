# FSWebServer – API (overview)

This page summarizes the main methods exposed by `FSWebServer` and when to use them.

> Note: some APIs are available only when the related features are enabled via macros (e.g. `ESP_FS_WS_SETUP`, `ESP_FS_WS_EDIT`).

## Constructor

```cpp
FSWebServer(uint16_t port, fs::FS &fs, const char* hostname = "");
```

- `port`: HTTP port (typically `80`)
- `fs`: filesystem (`LittleFS`, `SPIFFS`, `FFat`, …)
- `hostname`: optional (also used for mDNS)

## Server start

```cpp
void begin(WebSocketsServer::WebSocketServerEvent wsEventHandler = nullptr);
```

- Registers built-in handlers (setup/edit if enabled), static file serving, and notFound.
- If `wsEventHandler != nullptr`, creates and starts a websocket server.
- Starts the webserver.

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
WebSocketsServer* getWebSocketServer();
bool broadcastWebSocket(const String &payload);
bool broadcastWebSocket(const uint8_t *payload, size_t length);
bool sendWebSocket(uint8_t num, const String &payload);
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

// attach a comment string to an existing option element
// For most controls the comment is rendered in a separate line under the input.
// Boolean (checkbox) fields use a <span> so the text stays on the same line.
void addComment(const char *lbl, const char *comment);

// boolean option: grouped by default, pass fourth argument false to keep declaration order
// the third parameter specifies `hidden` exactly like the generic template
void addOption(const char *lbl, bool val, bool hidden = false, bool grouped = true);

// boolean overload that accepts a comment
void addOption(const char *lbl, bool val, const char *comment,
               bool hidden = false, bool grouped = true);

// generic templated version (bool excluded via SFINAE below)
template <typename T>
void addOption(const char *lbl, T val, bool hidden=false, double min=MIN_F, double max=MAX_F, double st=1.0);

// convenience comment overload for non-bool types
// use the bool-specific variant to add a comment to a checkbox
template <typename T, typename std::enable_if<!std::is_same<T,bool>::value, int>::type = 0>
void addOption(const char *lbl, T val, const char *comment);

template <typename T>
void addOption(const char *lbl, T val, bool hidden=false, double min=MIN_F, double max=MAX_F, double st=1.0);

template <typename T>
bool getOptionValue(const char *lbl, T &var);

template <typename T>
bool saveOptionValue(const char *lbl, T val);
```

Boolean options are now controlled per-option; the third parameter (or the `grouped` argument) determines whether the switch/check is collected with other booleans or left inline. Hidden behaviour still works via `hidden` argument.

Dropdown/Slider:

```cpp
using DropdownList = FSWebServer::DropdownList;
using Slider = FSWebServer::Slider;

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
