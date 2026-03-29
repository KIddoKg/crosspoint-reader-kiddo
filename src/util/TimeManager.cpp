#include "TimeManager.h"
#include <Logging.h>
#include <cstdio>
#include <cstring>
#include <sys/time.h>
#include <time.h>
#include <Arduino.h>
#include <freertos/task.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "CrossPointSettings.h"

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
        int gmtOffset = 0;
        HTTPClient http;
        http.setTimeout(5000);
        http.setConnectTimeout(5000);
        
        if (http.begin("http://ip-api.com/json?fields=timezone")) {
          int httpCode = http.GET();
          if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);
            if (!error && doc["timezone"].is<const char*>()) {
              const char* timezone = doc["timezone"].as<const char*>();
              std::string tzStr(timezone);
              if (tzStr.find("Ho_Chi_Minh") != std::string::npos ||
                  tzStr.find("Bangkok") != std::string::npos ||
                  tzStr.find("Hanoi") != std::string::npos) {
                gmtOffset = 7 * 3600;
              } else if (tzStr.find("Shanghai") != std::string::npos ||
                         tzStr.find("Hong_Kong") != std::string::npos) {
                gmtOffset = 8 * 3600;
              } else if (tzStr.find("Tokyo") != std::string::npos) {
                gmtOffset = 9 * 3600;
              } else if (tzStr.find("Sydney") != std::string::npos) {
                gmtOffset = 10 * 3600;
              } else if (tzStr.find("New_York") != std::string::npos ||
                         tzStr.find("America") != std::string::npos) {
                gmtOffset = -5 * 3600;
              } else if (tzStr.find("Los_Angeles") != std::string::npos) {
                gmtOffset = -8 * 3600;
              }
            }
          }
          http.end();
        }
        
        configTime(gmtOffset, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");
        SETTINGS.timezoneOffsetSeconds = gmtOffset;
        SETTINGS.saveToFile();
        
        int attempts = 0;
        time_t now = time(nullptr);
        while (now < 24 * 3600 && attempts < 300) {
          vTaskDelay(pdMS_TO_TICKS(100));
          now = time(nullptr);
          attempts++;
        }
        
        if (now > 24 * 3600) {
          pThis->lastSyncTime = now;
          pThis->isTimeSet = true;
          SETTINGS.manualTimeBase = now;
          SETTINGS.saveToFile();
        }
        pThis->isSyncing = false; // Kết thúc đồng bộ
        vTaskDelete(nullptr);
      },
      "NTPSync",
      4096,
      this,
      1,
      nullptr);
}

void TimeManager::restoreTimezoneFromSettings() {
  if (SETTINGS.timezoneOffsetSeconds != 0) {
    configTime(SETTINGS.timezoneOffsetSeconds, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");
  }
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
