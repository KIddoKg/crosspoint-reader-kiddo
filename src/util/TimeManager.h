#pragma once
#include <ctime>
#include <string>

class TimeManager {
 private:
  time_t lastSyncTime = 0;
  bool hasWiFi = false;
  bool isTimeSet = false;
  bool isSyncing = false; // Thêm cờ theo dõi trạng thái đồng bộ NTP

  // When setting manual date/time we store a base time and the system time when it was set
  // so we can advance the manual time as the system clock ticks.
  time_t manualBaseTime = 0;   // the epoch time corresponding to the manual date/time
  time_t manualSetAt = 0;      // system epoch when manualBaseTime was recorded

  TimeManager() = default;

 public:
  static TimeManager& getInstance() {
    static TimeManager instance;
    return instance;
  }

  std::string getCurrentTimeString() const;
  std::string getTimeString() const;  // Alias for convenience
  std::string getCurrentDateString() const;
  time_t getCurrentTime() const;

  // Offline mode: set time manually
  void setManualTime(int hour, int minute, int second);

  // Set full manual date and time (year, month, day, hour, minute, second)
  void setManualDateTime(int year, int month, int day, int hour, int minute, int second);

  // Online mode: sync from NTP server
  void syncTimeFromNTP();

  // Restore timezone offset from settings (call during boot)
  void restoreTimezoneFromSettings();

  void setWiFiConnected(bool connected);
  bool isWiFiConnected() const { return hasWiFi; }
  bool hasWiFiConnection() const { return hasWiFi; }  // Alias for convenience
  bool isTimeValid() const { return isTimeSet; }
  bool isTimeSyncing() const { return isSyncing; } // Kiểm tra đang đồng bộ NTP

  // Utility
  int getHour() const;
  int getMinute() const;
  int getSecond() const;

  // Fetch weather information from wttr.in
  void fetchWeatherData();
};
