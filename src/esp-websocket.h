#ifndef ESP_WEBSOCKET_H_
#define ESP_WEBSOCKET_H_

#include <functional>

#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#include "SerialLog.h"
#include "websocket/WebSocketsServer.h"


class ServerWebSocket : public WebSocketsServer {
public:
    using WsConnect_cb       = std::function<void(uint8_t)>;
    using WsReceive_cb       = std::function<void(uint8_t, uint8_t*, size_t)>;

    ServerWebSocket(uint16_t port, const String& origin = "", const String& protocol = "arduino")
        : WebSocketsServer(port, origin, protocol) {
            using namespace std::placeholders;

            this->begin();
            this->onEvent(std::bind(&ServerWebSocket::webSocketEvent, this, _1, _2, _3, _4));
        }

    ~ServerWebSocket(void) {;}

    void onWebsocketConnect(WsConnect_cb fn) {
        onConnect = fn;
    }

    void onWebsocketDisconnect(WsConnect_cb fn) {
        onDisconnect = fn;
    }

    void onWebsocketReceive(WsReceive_cb fn) {
        onReceive = fn;
    }
	
	
    void print(const char* data) {
        this->broadcastTXT(data, strlen(data));
    }

    void print(String& data) {
        this->broadcastTXT(data);
    }

protected:
    WsConnect_cb onConnect = nullptr;
    WsConnect_cb onDisconnect = nullptr;
    WsReceive_cb onReceive = nullptr;

    void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t len) {
        switch (type) {
        case WStype_DISCONNECTED:
            log_debug("Websocket client %d disconnected", num);
            if (onDisconnect != nullptr)
                onDisconnect(num);
            break;
        case WStype_CONNECTED: {
            log_debug("Websocket client %d connected", num);
            if (onConnect != nullptr)
                onConnect(num);
        } break;
        case WStype_TEXT:
            log_debug("Websocket client %d message: %s", num, payload);
            if (onReceive != nullptr)
                onReceive(num, payload, len);
            break;
        default:
            break;
        }
    }


};

#endif