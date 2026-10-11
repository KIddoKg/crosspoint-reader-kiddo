#pragma once
#include <cstdint>
#include <iosfwd>
#include <string>

class CrossPointState {
  // Static instance
  static CrossPointState instance;

 public:
  std::string openEpubPath;
  uint8_t lastSleepImage;
  uint8_t readerActivityLoadCount = 0;
  bool lastSleepFromReader = false;
  // Wall-clock time when the device last went to sleep; lets the clock resume roughly where it was after a
  // deep sleep that powered off the RTC domain (better than jumping back to the last WiFi sync)
  int64_t lastSleepTime = 0;
  // Last automatic background time sync attempt (see BackgroundTimeSync)
  int64_t lastAutoSyncTime = 0;
  // Last fetched weather ("City\n+28°C Cloudy"), shown until the next fetch
  std::string lastWeather;
  ~CrossPointState() = default;

  // Get singleton instance
  static CrossPointState& getInstance() { return instance; }

  bool saveToFile() const;

  bool loadFromFile();

 private:
  bool loadFromBinaryFile();
};

// Helper macro to access settings
#define APP_STATE CrossPointState::getInstance()
