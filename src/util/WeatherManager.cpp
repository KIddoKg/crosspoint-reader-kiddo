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
    // When WiFi connects, fetch weather data in background
    // Delay slightly to avoid conflicts with reader task
    hasWiFi = true;
    LOG_INF("WEATHER", "WiFi connected, fetching weather data...");
    
    // Schedule weather fetch with a small delay to avoid filesystem conflicts
    xTaskCreate(
        [](void* param) {
          WeatherManager* pThis = static_cast<WeatherManager*>(param);
          vTaskDelay(pdMS_TO_TICKS(500));  // Wait 500ms before fetching
          pThis->fetchWeatherData();
          vTaskDelete(nullptr);
        },
        "WeatherScheduler",
        4096,
        this,
        3,  // Lower priority than reader
        nullptr);
  } else {
    // Cancel any in-flight fetch when WiFi disconnects
    LOG_INF("WEATHER", "WiFi disconnected, cancelling weather fetch");
    cancelFetchTask();
    hasWiFi = false;
    setDefaultValues();
  }
}

void WeatherManager::fetchWeatherData() {
  if (!hasWiFi) {
    LOG_ERR("WEATHER", "WiFi not connected, cannot fetch weather");
    return;
  }

  // Cancel any previous fetch task before starting a new one
  cancelFetchTask();
  shouldCancelFetch = false;

  LOG_INF("WEATHER", "Fetching weather data from wttr.in...");
  
  // Create a background task to fetch weather data
  // Use higher priority to avoid blocking, but lower stack to save memory
  xTaskCreate(
      [](void* param) {
        WeatherManager* pThis = static_cast<WeatherManager*>(param);
        
        HTTPClient http;
        http.setTimeout(3000);  // Reduce timeout
        http.setConnectTimeout(3000);
        
        // Use HTTP instead of HTTPS to save memory and reduce resource usage
        // Format: "city:+temperature condition"
        if (http.begin("http://wttr.in/?format=%l:+%t+%C")) {
          int httpCode = http.GET();
          
          if (httpCode == HTTP_CODE_OK && !pThis->shouldCancelFetch) {
            String payload = http.getString();
            std::string rawData = payload.c_str();
            
            // Parse: "city:+temperature condition"
            size_t colonPos = rawData.find(':');
            if (colonPos != std::string::npos) {
              std::string city = rawData.substr(0, colonPos);
              std::string weather = rawData.substr(colonPos + 1);
              
              // Format city with Title Case
              city = titleCase(city);
              
              // Combine into 2 lines
              pThis->weatherData = city + "\n" + weather;
              
              LOG_INF("WEATHER", "Weather parsed - City: %s, Weather: %s", city.c_str(), weather.c_str());
            } else {
              pThis->weatherData = rawData;
              LOG_INF("WEATHER", "Weather fetched (raw): %s", pThis->weatherData.c_str());
            }
          } else if (pThis->shouldCancelFetch) {
            LOG_DBG("WEATHER", "Weather fetch cancelled");
          } else {
            LOG_ERR("WEATHER", "Failed to fetch weather, code: %d", httpCode);
            pThis->setDefaultValues();
          }
          
          http.end();
        } else {
          LOG_ERR("WEATHER", "Could not connect to wttr.in");
          pThis->setDefaultValues();
        }
        
        pThis->lastUpdateTime = millis();
        pThis->fetchTaskHandle = nullptr;
        vTaskDelete(nullptr);
      },
      "WeatherFetch",     // Task name
      12288,               // Stack size (reduced from 4096 to save memory)
      this,               // Parameter (this pointer)
      2,                  // Priority (2 = normal-low, won't block reader tasks)
      &fetchTaskHandle);  // Task handle
}

void WeatherManager::setDefaultValues() {
  weatherData = "N/A";
}

void WeatherManager::cancelFetchTask() {
  if (fetchTaskHandle != nullptr) {
    shouldCancelFetch = true;
    // Give task a chance to check shouldCancelFetch and exit gracefully
    // Wait up to 500ms for the task to exit on its own
    for (int i = 0; i < 5 && fetchTaskHandle != nullptr; i++) {
      vTaskDelay(pdMS_TO_TICKS(100));
    }
    // If task is still running after timeout, force delete it
    if (fetchTaskHandle != nullptr) {
      vTaskDelete(fetchTaskHandle);
      fetchTaskHandle = nullptr;
      LOG_ERR("WEATHER", "Weather fetch task force deleted due to timeout");
    }
    LOG_DBG("WEATHER", "Weather fetch task cancelled");
  }
}
