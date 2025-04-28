idf_component_register(
    SRC_DIRS "src" "src/websocket" "src/websocket/libb64" "src/websocket/libsha1"
    INCLUDE_DIRS "src" "src/websocket" "src/websocket/libb64" "src/websocket/libsha1"
    REQUIRES arduino-esp32 ArduinoJson
)
 