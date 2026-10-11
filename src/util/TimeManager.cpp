#include "TimeManager.h"
#include <Logging.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <sys/time.h>
#include <time.h>
#include <Arduino.h>
#include <freertos/task.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_sntp.h>
// GPIO13 hold giữ LDO sống trong deep sleep → RTC domain alive
// → esp_rtc_get_time_us() đếm đúng qua sleep
extern "C" uint64_t esp_rtc_get_time_us(void);
#include "CrossPointSettings.h"
#include "CrossPointState.h"

// Biến trong RTC Memory của ESP32-C3 (LP SRAM)
// → survive deep sleep vì GPIO13 hold giữ LDO sống
RTC_DATA_ATTR static time_t  g_rtcSavedTime    = 0;
RTC_DATA_ATTR static bool    g_rtcTimeValid    = false;
RTC_DATA_ATTR static int64_t g_rtcTimerAtSleep = 0;  // LP timer (us) lúc vào sleep

std::string TimeManager::getCurrentTimeString() const {
  time_t now = getCurrentTime();
  struct tm* timeinfo = localtime(&now);
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
  return std::string(buffer);
}

std::string TimeManager::getTimeString() const { return getCurrentTimeString(); }

std::string TimeManager::getCurrentDateString() const {
  time_t now = getCurrentTime();
  struct tm* timeinfo = localtime(&now);
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);
  return std::string(buffer);
}

time_t TimeManager::getCurrentTime() const {
  return time(nullptr);
}

bool TimeManager::hasValidClock() const {
  constexpr time_t MIN_VALID_TIME = 1704067200;  // 2024-01-01
  return time(nullptr) > MIN_VALID_TIME;
}

bool TimeManager::formatClock(char* buffer, const size_t size) const {
  if (!hasValidClock()) {
    return false;
  }
  const time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  // "~" marks an estimated time (restored from SD after a deep sleep without RTC) until the next sync
  strftime(buffer, size, approximate ? "~%H:%M" : "%H:%M", &timeinfo);
  return true;
}

void TimeManager::setManualTime(int hour, int minute, int second) {
  LOG_INF("TIME", "Setting manual time: %02d:%02d:%02d", hour, minute, second);

  time_t now = time(nullptr);
  struct tm* t = localtime(&now);
  t->tm_hour = hour;
  t->tm_min = minute;
  t->tm_sec = second;

  time_t newTime = mktime(t);
  struct timeval tv = {.tv_sec = newTime, .tv_usec = 0};
  settimeofday(&tv, nullptr);

  SETTINGS.manualTimeBase = newTime;
  SETTINGS.saveToFile();

  isTimeSet = true;
  approximate = false;
}

void TimeManager::setManualDateTime(int year, int month, int day, int hour, int minute, int second) {
  LOG_INF("TIME", "Setting manual datetime: %04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);

  struct tm tmVal;
  memset(&tmVal, 0, sizeof(tmVal));
  tmVal.tm_year = year - 1900;
  tmVal.tm_mon = month - 1;
  tmVal.tm_mday = day;
  tmVal.tm_hour = hour;
  tmVal.tm_min = minute;
  tmVal.tm_sec = second;
  tmVal.tm_isdst = -1;

  time_t newTime = mktime(&tmVal);
  struct timeval tv = {.tv_sec = newTime, .tv_usec = 0};
  settimeofday(&tv, nullptr);

  SETTINGS.manualTimeBase = newTime;
  SETTINGS.saveToFile();

  isTimeSet = true;
  approximate = false;
}

void TimeManager::syncTimeFromNTP() {
  if (!hasWiFi) {
    LOG_ERR("TIME", "WiFi not connected, cannot sync NTP");
    return;
  }

  isSyncing = true; // Bắt đầu đồng bộ
  LOG_INF("TIME", "Starting time sync with timezone detection...");
  
  xTaskCreate(
      [](void* param) {
        TimeManager* pThis = static_cast<TimeManager*>(param);
        // Keep the saved offset unless the lookup succeeds (a failed lookup used to reset it to UTC)
        int gmtOffset = SETTINGS.timezoneOffsetSeconds;
        HTTPClient http;
        http.setTimeout(5000);
        http.setConnectTimeout(5000);

        // ip-api returns the UTC offset (seconds, DST included) of the request IP's timezone
        if (http.begin("http://ip-api.com/json?fields=status,offset")) {
          if (http.GET() == HTTP_CODE_OK) {
            JsonDocument doc;
            if (!deserializeJson(doc, http.getString()) && doc["status"] == "success" && doc["offset"].is<int>()) {
              gmtOffset = doc["offset"].as<int>();
            }
          }
          http.end();
        }

        // Khởi động SNTP và cấu hình timezone
        configTime(gmtOffset, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");
        SETTINGS.timezoneOffsetSeconds = gmtOffset;
        SETTINGS.saveToFile();

        // Dùng sntp_get_sync_status() thay vì check time() để tránh false-positive
        // khi time đã hợp lệ từ RTC restore trước đó
        int attempts = 0;
        while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED && attempts < 300) {
          vTaskDelay(pdMS_TO_TICKS(100));
          attempts++;
        }
        time_t now = time(nullptr);

        // Judge success by the SNTP status: time() is already plausible when it was restored approximately
        if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED && now > 24 * 3600) {
          pThis->approximate = false;
          pThis->lastSyncTime = now;
          pThis->isTimeSet = true;
          SETTINGS.manualTimeBase = now;
          SETTINGS.saveToFile();
          // Cập nhật RTC memory ngay sau NTP sync để save được time mới nhất
          g_rtcSavedTime = now;
          g_rtcTimeValid = true;
          LOG_INF("TIME", "NTP sync OK: %ld (waited %d*100ms)", (long)now, attempts);
        } else {
          LOG_ERR("TIME", "NTP sync FAILED after %d attempts", attempts);
        }
        pThis->isSyncing = false;
        vTaskDelete(nullptr);
      },
      "NTPSync",
      4096,
      this,
      1,
      nullptr);
}

void TimeManager::restoreTimezoneFromSettings() {
  if (SETTINGS.timezoneOffsetSeconds == 0) return;

  // Chỉ set timezone, KHÔNG gọi configTime()/sntp_init() ở đây
  // Lý do: configTime() khởi động SNTP trong background và có thể reset time()
  // về epoch=0 sau khi ta đã settimeofday() từ RTC memory.
  // SNTP chỉ được start khi WiFi kết nối thực sự (setWiFiConnected(true)).
  const int offsetSec = SETTINGS.timezoneOffsetSeconds;
  const int absHours  = abs(offsetSec) / 3600;
  const int absMins   = (abs(offsetSec) % 3600) / 60;
  // POSIX TZ: dấu ĐẢO NGƯỢC so với UTC offset
  // Ví dụ: UTC+7 → "UTC-7"
  char tzStr[16];
  if (absMins == 0) {
    snprintf(tzStr, sizeof(tzStr), "UTC%+d", -(offsetSec / 3600));
  } else {
    snprintf(tzStr, sizeof(tzStr), "UTC%c%d:%02d",
             (offsetSec >= 0 ? '-' : '+'), absHours, absMins);
  }
  setenv("TZ", tzStr, 1);
  tzset();
  LOG_DBG("TIME", "Timezone set to %s (offset=%ds)", tzStr, offsetSec);
}

// Lưu time vào RTC Memory trước khi vào deep sleep
void TimeManager::saveTimeToRTC() {
  time_t now = time(nullptr);
  if (now > 24 * 3600LL) {
    g_rtcSavedTime    = now;
    g_rtcTimeValid    = true;
    g_rtcTimerAtSleep = (int64_t)esp_rtc_get_time_us();
    LOG_INF("TIME", "Saved to RTC: %ld, lpMs=%d", (long)now, (int)(g_rtcTimerAtSleep / 1000));
  } else if (SETTINGS.manualTimeBase > 24 * 3600LL) {
    g_rtcSavedTime    = SETTINGS.manualTimeBase;
    g_rtcTimeValid    = true;
    g_rtcTimerAtSleep = (int64_t)esp_rtc_get_time_us();
    LOG_ERR("TIME", "time() invalid, fallback manualTimeBase: %ld", (long)SETTINGS.manualTimeBase);
  } else {
    g_rtcTimeValid    = false;
    g_rtcTimerAtSleep = 0;
    LOG_ERR("TIME", "No valid time to save");
  }
}

// Khôi phục time từ RTC Memory sau khi wake up
void TimeManager::restoreTimeFromRTC() {
  // Lớp 1: RTC Memory + elapsed (GPIO13 hold đảm bảo LP timer đếm đúng)
  if (g_rtcTimeValid && g_rtcSavedTime > 24 * 3600LL) {
    const int64_t nowUs      = (int64_t)esp_rtc_get_time_us();
    const int64_t elapsedUs  = (g_rtcTimerAtSleep > 0 && nowUs > g_rtcTimerAtSleep)
                                 ? (nowUs - g_rtcTimerAtSleep) : 0;
    const time_t  elapsedSec = (time_t)(elapsedUs / 1000000LL);
    const time_t  restored   = g_rtcSavedTime + elapsedSec;
    struct timeval tv = {.tv_sec = restored, .tv_usec = 0};
    settimeofday(&tv, nullptr);
    isTimeSet       = true;
    restoredFromRTC = true;
    LOG_INF("TIME", "Restored: %ld + sleep=%ds = %ld",
            (long)g_rtcSavedTime, (int)elapsedSec, (long)restored);
    return;
  }
  // Lớp 2: SD card (survive mất nguồn hoàn toàn). Use the newest of the last sync/manual time and the time the
  // device went to sleep; the clock will lag by the sleep duration until the next WiFi sync.
  const time_t sdTime = std::max<time_t>(SETTINGS.manualTimeBase, static_cast<time_t>(APP_STATE.lastSleepTime));
  if (sdTime > 24 * 3600LL) {
    struct timeval tv = {.tv_sec = sdTime, .tv_usec = 0};
    settimeofday(&tv, nullptr);
    isTimeSet = true;
    approximate = true;  // lags by however long the device slept
    LOG_INF("TIME", "Restored from SD: %ld", (long)sdTime);
    return;
  }
  LOG_DBG("TIME", "No valid time (never NTP synced)");
}


int TimeManager::getHour() const {
  time_t now = getCurrentTime();
  struct tm* timeinfo = localtime(&now);
  return timeinfo->tm_hour;
}

int TimeManager::getMinute() const {
  time_t now = getCurrentTime();
  struct tm* timeinfo = localtime(&now);
  return timeinfo->tm_min;
}

int TimeManager::getSecond() const {
  time_t now = getCurrentTime();
  struct tm* timeinfo = localtime(&now);
  return timeinfo->tm_sec;
}

void TimeManager::setWiFiConnected(bool connected) {
  hasWiFi = connected;
  if (connected) {
    syncTimeFromNTP();
  } else {
    isSyncing = false;
  }
}
