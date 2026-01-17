# Filesystem + Web File Editor (/edit)

The library serves static files from the filesystem and, optionally, includes a web editor (ACE) to manage files directly from the browser.

## Static file serving

By default, files are served from the filesystem with:

- root URL `/` -> filesystem path `/`
- default file: `index.htm`

In code (internally):

- `serveStatic("/", fs, "/").setDefaultFile("index.htm")`

## Stampa contenuto filesystem (debug)
## Print filesystem contents (debug)

```cpp
server.printFileList(FILESYSTEM, "/", 1);           // default -> Serial
server.printFileList(FILESYSTEM, "/", 1, Serial);  // or to a chosen stream
```

The `Print&` overload is useful to log to streams other than `Serial`.

## Enable /edit

```cpp
server.enableFsCodeEditor();
```

Main endpoints:
- `GET /edit` editor page
- `GET /list?dir=/` directory listing
- `POST /edit` upload
- `PUT /edit` create/rename
- `DELETE /edit` delete

## Provide filesystem info (recommended on ESP32)

On ESP32, to show correct “total/used bytes” in the UI:

```cpp
server.setFsInfoCallback([](fsInfo_t* fsInfo) {
  fsInfo->fsName = "LittleFS";
  fsInfo->totalBytes = LittleFS.totalBytes();
  fsInfo->usedBytes = LittleFS.usedBytes();
});
```
