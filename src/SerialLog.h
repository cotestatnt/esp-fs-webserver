#ifndef __SERIALLOG_H__
#define __SERIALLOG_H__

#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define DBG_OUTPUT_PORT     Serial
// Allow external override; default to verbose debug if not provided
#ifndef LOG_LEVEL
#define LOG_LEVEL           1    // (0 disable, 1 error, 2 info, 3 debug)
#endif

#define __SOURCE_FILE_NAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__))
#define _LOG_FORMAT(letter, format)  "\n["#letter"][%s:%u] %s():\t" format, __SOURCE_FILE_NAME__, __LINE__, __FUNCTION__

#if defined(LOG_LEVEL) && (LOG_LEVEL == 0)
#define log_error(format, ...)
#define log_info(format, ...)
#define log_debug(format, ...)
#endif

#if defined(LOG_LEVEL) && (LOG_LEVEL > 0)
#define log_error(format, ...) DBG_OUTPUT_PORT.printf(_LOG_FORMAT(E, format), ##__VA_ARGS__)
#define log_info(format, ...)
#define log_debug(format, ...)
#endif

#if defined(LOG_LEVEL) && (LOG_LEVEL > 1)
#undef log_info
#define log_info(format, ...) DBG_OUTPUT_PORT.printf(_LOG_FORMAT(I, format), ##__VA_ARGS__)
#endif

#if defined(LOG_LEVEL) && (LOG_LEVEL > 2)
#undef log_debug
#define log_debug(format, ...) DBG_OUTPUT_PORT.printf(_LOG_FORMAT(D, format), ##__VA_ARGS__)
#endif



#ifdef __cplusplus
}
#endif

#endif