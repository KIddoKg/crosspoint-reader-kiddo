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

  LOG_INF("TIME", "Starting time sync with timezone detection...");
  
  // Create a background task to sync time and detect timezone
  xTaskCreate(
      [](void* param) {
        TimeManager* pThis = static_cast<TimeManager*>(param);
        
        int gmtOffset = 0;  // Default to UTC
        
        // First, try to detect timezone from IP
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
              LOG_INF("TIME", "Detected timezone: %s", timezone);
              
              // Parse timezone string (e.g., "Asia/Ho_Chi_Minh") to GMT offset
              // Simple approximation based on common timezones
              std::string tzStr(timezone);
              
              if (tzStr.find("Ho_Chi_Minh") != std::string::npos || 
                  tzStr.find("Bangkok") != std::string::npos ||
                  tzStr.find("Hanoi") != std::string::npos) {
                gmtOffset = 7 * 3600;  // UTC+7
              } else if (tzStr.find("Shanghai") != std::string::npos ||
                         tzStr.find("Hong_Kong") != std::string::npos) {
                gmtOffset = 8 * 3600;  // UTC+8
              } else if (tzStr.find("Tokyo") != std::string::npos) {
                gmtOffset = 9 * 3600;  // UTC+9
              } else if (tzStr.find("Sydney") != std::string::npos) {
                gmtOffset = 10 * 3600;  // UTC+10
              } else if (tzStr.find("London") != std::string::npos ||
                         tzStr.find("GMT") != std::string::npos) {
                gmtOffset = 0;  // UTC+0
              } else if (tzStr.find("New_York") != std::string::npos ||
                         tzStr.find("America") != std::string::npos) {
                gmtOffset = -5 * 3600;  // UTC-5
              } else if (tzStr.find("Los_Angeles") != std::string::npos) {
                gmtOffset = -8 * 3600;  // UTC-8
              } else {
                gmtOffset = 0;  // Default to UTC
              }
              
              LOG_INF("TIME", "GMT offset: %d seconds", gmtOffset);
            } else {
              LOG_ERR("TIME", "Failed to parse timezone from response");
            }
          } else {
            LOG_ERR("TIME", "Failed to get timezone, code: %d", httpCode);
          }
          http.end();
        } else {
          LOG_ERR("TIME", "Could not connect to timezone API");
        }
        
        // Now configure NTP with the detected timezone
        LOG_INF("TIME", "Configuring NTP with timezone offset...");
        configTime(gmtOffset, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");
        
        // Save timezone offset for persistence across reboots
        SETTINGS.timezoneOffsetSeconds = gmtOffset;
        SETTINGS.saveToFile();
        
        // Wait for NTP sync to complete (up to 30 seconds)
        int attempts = 0;
        time_t now = time(nullptr);
        
        while (now < 24 * 3600 && attempts < 300) {
          vTaskDelay(pdMS_TO_TICKS(100));
          now = time(nullptr);
          attempts++;
        }
        
        if (now > 24 * 3600) {
          // Time was synced successfully
          pThis->lastSyncTime = now;
          pThis->isTimeSet = true;
          
          // Save the synced time to settings
          SETTINGS.manualTimeBase = now;
          SETTINGS.saveToFile();
          
          LOG_INF("TIME", "Time sync successful: %ld (with timezone), saved to settings", now);
        } else {
          LOG_ERR("TIME", "NTP sync timeout after %d attempts", attempts);
          pThis->isTimeSet = true;
        }
        
        vTaskDelete(nullptr);
      },
      "NTPSync",          // Task name
      8192,               // Stack size (increased for HTTP operations)
      this,               // Parameter (this pointer)
      1,                  // Priority (low)
      nullptr);           // Task handle
  
  LOG_INF("TIME", "Time sync task started - will sync with timezone detection in background");
  isTimeSet = true;
}

void TimeManager::restoreTimezoneFromSettings() {
  // Restore timezone offset that was previously saved during WiFi sync
  // This ensures timezone persists across reboots
  if (SETTINGS.timezoneOffsetSeconds != 0) {
    LOG_INF("TIME", "Restoring timezone offset: %d seconds", SETTINGS.timezoneOffsetSeconds);
    configTime(SETTINGS.timezoneOffsetSeconds, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");
  } else {
    LOG_DBG("TIME", "No saved timezone offset, using UTC by default");
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
  
  // When WiFi connects, automatically sync time from NTP
  if (connected) {
    LOG_INF("TIME", "WiFi connected, syncing time from NTP...");
    syncTimeFromNTP();
  }
}
