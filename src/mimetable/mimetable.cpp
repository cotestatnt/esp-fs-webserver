#include "mimetable.h"
#include <pgmspace.h>

namespace mimetype {

// MIME type lookup table stored in PROGMEM
static const struct {
  const char ext[16];
  const char type[32];
} mimeTable[] PROGMEM = {
  {".html", "text/html"},
  {".htm", "text/html"},
  {".css", "text/css"},
  {".txt", "text/plain"},
  {".js", "application/javascript"},
  {".mjs", "text/javascript"},
  {".json", "application/json"},
  {".png", "image/png"},
  {".gif", "image/gif"},
  {".jpg", "image/jpeg"},
  {".ico", "image/x-icon"},
  {".svg", "image/svg+xml"},
  {".svg.gz", "image/svg+xml"},
  {".ttf", "application/x-font-ttf"},
  {".otf", "application/x-font-opentype"},
  {".woff", "application/font-woff"},
  {".woff2", "application/font-woff2"},
  {".eot", "application/vnd.ms-fontobject"},
  {".sfnt", "application/font-sfnt"},
  {".xml", "text/xml"},
  {".pdf", "application/pdf"},
  {".zip", "application/zip"},
  {".gz", "application/x-gzip"},
  {".appcache", "text/cache-manifest"},
};

String getContentType(const String &path) {
  char buff[32];
  
  // Check all entries for extension match
  for (size_t i = 0; i < sizeof(mimeTable) / sizeof(mimeTable[0]); i++) {
    strcpy_P(buff, mimeTable[i].ext);
    if (path.endsWith(buff)) {
      strcpy_P(buff, mimeTable[i].type);
      return String(buff);
    }
  }
  
  // Default to octet-stream if no match
  return String(F("application/octet-stream"));
}

}  // namespace mimetype
