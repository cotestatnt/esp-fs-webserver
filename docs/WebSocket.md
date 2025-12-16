# WebSocket Support

The library includes and uses [WebSockets](https://github.com/Links2004/arduino-WebSockets) by Markus Sattler for bidirectional communication with web clients.

Support is enabled by default via the `ESP_FS_WS_WEBSOCKET` macro.

## 1. Create the Event Handler

First, you need to define a function that will handle WebSocket events. This function must match the `WebSocketServerEvent` signature.

```cpp
// WebSocket event handler
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = server.getWebSocketServer()->remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // Send a welcome message to the newly connected client
        server.getWebSocketServer()->sendTXT(num, "Hello from FSWebServer!");
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] Received text: %s\n", num, payload);

      // Echo the message back to the client
      server.getWebSocketServer()->sendTXT(num, payload);
      break;
    case WStype_BIN:
      Serial.printf("[%u] Received binary data of length %u\n", num, length);
      // hexdump(payload, length); // Example for printing binary data
      break;
    default:
      Serial.printf("Unhandled event type: %d\n", type);
      break;
  }
}
```
- `num`: The client ID (an integer from 0 up to the maximum number of clients).
- `type`: The type of event (e.g., `WStype_CONNECTED`, `WStype_DISCONNECTED`, `WStype_TEXT`).
- `payload`: A pointer to the received data.
- `length`: The length of the received data.

## 2. Start the Server with the Handler

Pass your event handler function to the `server.begin()` method. This will automatically start the WebSocket server.

```cpp
void setup() {
  // ... other setup code ...

  // Start the web server and enable WebSockets
  server.begin(webSocketEvent);
}
```

## 3. Sending Messages from the ESP

You have two ways to send messages to clients.

### Simple Broadcast (Recommended)

Use the helper methods built into `FSWebServer` to easily broadcast messages to all connected clients.

```cpp
// Broadcast a text message
server.broadcastWebSocket("This is a message for everyone.");

// Broadcast binary data
uint8_t binaryPayload[] = {0xDE, 0xAD, 0xBE, 0xEF};
server.broadcastWebSocket(binaryPayload, sizeof(binaryPayload));
```

### Advanced Control

For more advanced scenarios, like sending a message to a specific client, you can get a pointer to the underlying `WebSocketsServer` object.

```cpp
// Get the WebSocket server instance
WebSocketsServer* ws = server.getWebSocketServer();

if (ws) {
  // Send a text message to client number 2
  ws->sendTXT(2, "This is a private message for you.");

  // Disconnect client number 3
  ws->disconnect(3);
}
```
This gives you full access to the underlying WebSocket library's API.
