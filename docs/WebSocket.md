# WebSocket (enable + broadcast)

The library can use `ESPAsyncWebServer` + `AsyncWebSocket` for bidirectional communication with web clients.

## Start with a custom handler (recommended)

```cpp
void onWsEvent(AsyncWebSocket* server,
               AsyncWebSocketClient* client,
               AwsEventType type,
               void* arg,
               uint8_t* data,
               size_t len) {
  // handle connect/disconnect/data...
}

server.init(onWsEvent);
```

With `init(onWsEvent)` a websocket is created on `/ws`.

## Enable WebSocket at runtime

If you want to create a websocket at a different time:

```cpp
server.enableWebSocket("/ws", onWsEvent);
```

## Send messages to all clients

Text:

```cpp
server.wsBroadcast("hello");
```

Binary:

```cpp
uint8_t payload[] = {0x01, 0x02};
server.wsBroadcastBinary(payload, sizeof(payload));
```

## Get the websocket pointer

```cpp
AsyncWebSocket* ws = server.getWebSocket();
```

Useful if you want to manage websocket options directly (ping, cleanup clients, etc.).
