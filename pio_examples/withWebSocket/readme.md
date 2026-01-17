## AsyncFsWebServer – withWebSocket example

This folder contains a PlatformIO example and a small documentation set for the `AsyncFsWebServer` library.

### Documentation (available methods + usage)

- [docs/README.md](../../docs/README.md)
- [docs/API.md](../../docs/API.md)
- [docs/SetupAndWiFi.md](../../docs/SetupAndWiFi.md)
- [docs/FileEditorAndFS.md](../../docs/FileEditorAndFS.md)
- [docs/WebSocket.md](../../docs/WebSocket.md)

### Quick start (summary)

```cpp
AsyncFsWebServer server(80, FILESYSTEM, hostname);

if (FILESYSTEM.begin()) {
	server.printFileList(FILESYSTEM, "/", 1); // default -> Serial
}

if (!server.startWiFi(10000)) {
	server.startCaptivePortal("ESP_AP", "123456789", "/setup");
}

server.enableFsCodeEditor();
server.init(onWsEvent);
```

---

## Original note (example description)
This example is a bit more advanced than the [simpleServer](https://github.com/cotestatnt/esp-fs-webserver/tree/main/examples/simpleServer) example.

Basically, it uses the same HTML as simpleServer, but with the addition of a **WebSocket client** in the web page.

With WebSocket technology, we can send messages from server-to-client or client-to-server in a **full-duplex communication channel over a single TCP connection.**

In this simple example, it is used only to push a message from the ESP (the NTP-synchronized system time) to connected clients, just to show how to set up a system like this.

![image](https://user-images.githubusercontent.com/27758688/151001497-6468b50f-d4cb-46e1-ab4c-4d7aca3883db.png)
