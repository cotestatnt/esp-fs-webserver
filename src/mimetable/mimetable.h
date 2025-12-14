#ifndef __MIMETABLE_H__
#define __MIMETABLE_H__
#include <Arduino.h>

namespace mimetype {

// Minimal MIME type helper - delegates to ESP32 WebServer core when possible
// Falls back to local implementation if WebServer::getContentType is unavailable
String getContentType(const String &path);

}  // namespace mimetype

#endif
