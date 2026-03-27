#pragma once

#include <functional>
#include <string>

#include "activities/ActivityWithSubactivity.h"
#include "util/ButtonNavigator.h"

/**
 * FeaturesActivity displays device information:
 * - Current date and time
 * - Weather information (when WiFi is connected)
 * - Set time manually when WiFi is not available
 */
class FeaturesActivity final : public ActivityWithSubactivity {
 public:
  explicit FeaturesActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                           const std::function<void()>& onBack)
      : ActivityWithSubactivity("Features", renderer, mappedInput), onBack(onBack) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(Activity::RenderLock&&) override;

 private:
  ButtonNavigator buttonNavigator;
  const std::function<void()> onBack;

  enum class FeatureMenu {DATE, TIME, AREA, WEATHER, TEMPERATURE, COUNT };

  FeatureMenu selectedMenu = FeatureMenu::DATE;
  std::string currentDate;
  std::string currentTime;
  std::string weatherArea;
  std::string weatherCondition;
  std::string weatherTemperature;
  bool wifiConnected = false;
  unsigned long lastUpdateTime = 0;
  static constexpr unsigned long UPDATE_INTERVAL = 60000;  // Update every 60 seconds

  // Editable fields for offline manual date/time adjustment
  int editDay = 1;
  int editMonth = 1;
  int editYear = 1970;
  int editHour = 0;
  int editMinute = 0;
  int editSecond = 0;

  // Which sub-field is currently selected when editing date/time
  // dateField: 0=day,1=month,2=year; timeField: 0=hour,1=minute,2=second
  int dateField = 0;
  int timeField = 0;

  // Whether we're currently editing a sub-field (entered by pressing Confirm on a DATE/TIME item)
  bool editing = false;

  void updateDateTime();
  void updateWeather();
};
