#pragma once
#include <string>
#include <Arduino.h>
#include <freertos/task.h>

class WeatherManager {
 private:
  std::string weatherData;      // Full weather string from wttr.in (e.g., "⛅  +32°C")
  std::string city = "Unknown";  // City detected from IP
  unsigned long lastUpdateTime = 0;
  bool hasWiFi = false;
  bool isSyncing = false;  // Thêm cờ theo dõi trạng thái đồng bộ

  // Task handle to track background fetch and allow cancellation
  TaskHandle_t fetchTaskHandle = nullptr;
  bool shouldCancelFetch = false;

  WeatherManager() = default;

 public:
  static WeatherManager& getInstance() {
    static WeatherManager instance;
    return instance;
  }

  // Get the weather data from wttr.in
  std::string getWeatherData() const { return weatherData; }
  std::string getCity() const { return city; }

  // Fetch weather from wttr.in API
  void fetchWeatherData();

  void setWiFiConnected(bool connected);
  bool isWiFiConnected() const { return hasWiFi; }
  bool hasWiFiConnection() const { return hasWiFi; }  // Alias for convenience
  bool isWeatherSyncing() const { return isSyncing; } // Kiểm tra đang đồng bộ

  void setCity(const std::string& newCity) { city = newCity; }

 private:
  void setDefaultValues();
  void cancelFetchTask();  // Cancel any in-flight fetch task
};
