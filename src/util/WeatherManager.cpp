#include "WeatherManager.h"
#include <Arduino.h>
#include <freertos/task.h>
#include <Logging.h>
#include <HTTPClient.h>
#include <algorithm>
#include <cctype>

static std::string titleCase(const std::string& str) {
  std::string result = str;
  bool capitalizeNext = true;
  
  for (size_t i = 0; i < result.length(); i++) {
    if (std::isspace(result[i])) {
      capitalizeNext = true;
    } else if (capitalizeNext) {
      result[i] = std::toupper(result[i]);
      capitalizeNext = false;
    } else {
      result[i] = std::tolower(result[i]);
    }
  }
  
  return result;
}

void WeatherManager::setWiFiConnected(bool connected) {
  if (connected) {
    hasWiFi = true;
    LOG_INF("WEATHER", "WiFi connected, fetching weather data...");
    fetchWeatherData(); // Gọi trực tiếp không qua scheduler task trung gian
  } else {
    LOG_INF("WEATHER", "WiFi disconnected, cancelling weather fetch");
    cancelFetchTask();
    hasWiFi = false;
    isSyncing = false;
    setDefaultValues();
  }
}

void WeatherManager::fetchWeatherData() {
  if (!hasWiFi) {
    LOG_ERR("WEATHER", "WiFi not connected, cannot fetch weather");
    return;
  }

  cancelFetchTask();
  shouldCancelFetch = false;
  isSyncing = true; // Bắt đầu đồng bộ

  LOG_INF("WEATHER", "Fetching weather data from wttr.in...");
  
  xTaskCreate(
      [](void* param) {
        WeatherManager* pThis = static_cast<WeatherManager*>(param);
        
        HTTPClient http;
        http.setTimeout(5000);
        http.setConnectTimeout(5000);
        
        if (http.begin("http://wttr.in/?format=%l:+%t+%C")) {
          int httpCode = http.GET();
          
          if (httpCode == HTTP_CODE_OK && !pThis->shouldCancelFetch) {
            String payload = http.getString();
            std::string rawData = payload.c_str();
            
            size_t colonPos = rawData.find(':');
            if (colonPos != std::string::npos) {
              std::string cityStr = rawData.substr(0, colonPos);
              std::string weather = rawData.substr(colonPos + 1);
              cityStr = titleCase(cityStr);
              pThis->weatherData = cityStr + "\n" + weather;
            } else {
              pThis->weatherData = rawData;
            }
          } else {
            pThis->setDefaultValues();
          }
          http.end();
        } else {
          pThis->setDefaultValues();
        }
        
        pThis->lastUpdateTime = millis();
        pThis->isSyncing = false; // Kết thúc đồng bộ
        pThis->fetchTaskHandle = nullptr;
        vTaskDelete(nullptr);
      },
      "WeatherFetch",
      8192,
      this,
      2,
      &fetchTaskHandle);
}

void WeatherManager::setDefaultValues() {
  weatherData = "N/A";
}

void WeatherManager::cancelFetchTask() {
  if (fetchTaskHandle != nullptr) {
    shouldCancelFetch = true;
    for (int i = 0; i < 5 && fetchTaskHandle != nullptr; i++) {
      vTaskDelay(pdMS_TO_TICKS(100));
    }
    if (fetchTaskHandle != nullptr) {
      vTaskDelete(fetchTaskHandle);
      fetchTaskHandle = nullptr;
    }
    isSyncing = false;
  }
}
