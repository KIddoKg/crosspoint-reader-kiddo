#include "DebugFileLog.h"

#include <Arduino.h>
#include <HalStorage.h>
#include <Logging.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <time.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>

namespace {
constexpr size_t MAX_LOG_SIZE = 256 * 1024;
constexpr const char* OLD_LOG_PATH = "/.crosspoint/debug.old.log";

SemaphoreHandle_t logMutex = nullptr;
}  // namespace

void debugFileLog(const char* tag, const char* format, ...) {
  if (!logMutex) {
    logMutex = xSemaphoreCreateMutex();
  }
  if (!Storage.ready()) {
    return;
  }

  char msg[256];
  va_list args;
  va_start(args, format);
  vsnprintf(msg, sizeof(msg), format, args);
  va_end(args);

  char timeStr[24] = "-";
  const time_t now = time(nullptr);
  if (now > 24 * 3600) {
    struct tm t;
    localtime_r(&now, &t);
    strftime(timeStr, sizeof(timeStr), "%m-%d %H:%M:%S", &t);
  }

  char line[320];
  const int len = snprintf(line, sizeof(line), "[%s +%lums] %s: %s\n", timeStr, millis(), tag, msg);

  xSemaphoreTake(logMutex, portMAX_DELAY);
  Storage.mkdir("/.crosspoint");
  FsFile f = Storage.open(DEBUG_FILE_LOG_PATH, O_WRONLY | O_CREAT | O_APPEND);
  if (f) {
    if (f.size() > MAX_LOG_SIZE) {
      f.close();
      Storage.remove(OLD_LOG_PATH);
      Storage.rename(DEBUG_FILE_LOG_PATH, OLD_LOG_PATH);
      f = Storage.open(DEBUG_FILE_LOG_PATH, O_WRONLY | O_CREAT | O_APPEND);
    }
    if (f) {
      f.write(reinterpret_cast<const uint8_t*>(line), len > 0 ? std::min<size_t>(len, sizeof(line) - 1) : 0);
      f.close();
    }
  }
  xSemaphoreGive(logMutex);

  LOG_DBG("DLOG", "%s: %s", tag, msg);
}
