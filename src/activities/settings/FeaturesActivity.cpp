#include "FeaturesActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>
#include <ctime>
#include <sstream>
#include <sys/time.h>

#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "util/TimeManager.h"
#include "util/WeatherManager.h"
#include "activities/home/ReloadWiFiActivity.h"
#include "CrossPointSettings.h"

void FeaturesActivity::updateDateTime() {
  // If WiFi is connected we show real system time, otherwise show manual edit fields
  if (wifiConnected) {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    // Format date: "DD/MM/YYYY"
    char dateBuffer[16];
    strftime(dateBuffer, sizeof(dateBuffer), "%d/%m/%Y", timeinfo);
    currentDate = dateBuffer;

    // Format time: "HH:MM:SS"
    char timeBuffer[16];
    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", timeinfo);
    currentTime = timeBuffer;
  } else {
    // Build strings from editable fields
    char dateBuffer[16];
    snprintf(dateBuffer, sizeof(dateBuffer), "%02d/%02d/%04d", editDay, editMonth, editYear);
    currentDate = dateBuffer;

    char timeBuffer[16];
    snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d:%02d", editHour, editMinute, editSecond);
    currentTime = timeBuffer;
  }
}

void FeaturesActivity::updateWeather() {
  if (wifiConnected) {
    std::string weather = WeatherManager::getInstance().getWeatherData();
    LOG_INF("FEAT", "Raw weather data: '%s' (len=%zu)", weather.c_str(), weather.length());

    if (!weather.empty() && weather != "⛅  N/A") {

      // expected:
      // Ho Chi Minh City
      // +32°C Partly cloudy

      size_t newlinePos = weather.find('\n');

      if (newlinePos != std::string::npos) {
        weatherArea = weather.substr(0, newlinePos);
        std::string rest = weather.substr(newlinePos + 1);

        // trim leading spaces
        size_t start = 0;
        while (start < rest.length() && std::isspace(rest[start])) {
          start++;
        }
        rest = rest.substr(start);

        LOG_INF("FEAT", "Area: '%s'", weatherArea.c_str());
        LOG_INF("FEAT", "Rest: '%s'", rest.c_str());

        // parse: "+32°C Partly cloudy"
        std::istringstream iss(rest);

        iss >> weatherTemperature;        // +32°C
        std::getline(iss, weatherCondition); // " Partly cloudy"

        // trim condition leading space
        if (!weatherCondition.empty() && weatherCondition[0] == ' ') {
          weatherCondition.erase(0, 1);
        }

        LOG_INF("FEAT", "Parsed Temp='%s' Cond='%s'",
                weatherTemperature.c_str(),
                weatherCondition.c_str());

      } else {
        weatherArea = "N/A";
        weatherCondition = weather;
        weatherTemperature = "";
      }

    } else {
      weatherArea = "Fetching...";
      weatherCondition = "";
      weatherTemperature = "";
    }

  } else {
    weatherArea = "No WiFi";
    weatherCondition = "";
    weatherTemperature = "";
  }
}
void FeaturesActivity::onEnter() {
  ActivityWithSubactivity::onEnter();

  // Determine WiFi state from TimeManager
  wifiConnected = TimeManager::getInstance().isWiFiConnected();

  // Restore saved manual time if system time is not set (e.g. after reboot)
  time_t now = time(nullptr);
  if (now < 1000000 && SETTINGS.manualTimeBase > 1000000) {
      struct timeval tv = {.tv_sec = SETTINGS.manualTimeBase, .tv_usec = 0};
      settimeofday(&tv, nullptr);
      now = SETTINGS.manualTimeBase;
      LOG_INF("FEAT", "Restored time from settings: %ld", now);
  }

  // Restore selector state based on wifi
  selectedMenu = wifiConnected ? FeatureMenu::UPDATE : FeatureMenu::DATE;

  struct tm* timeinfo = localtime(&now);
  editDay = timeinfo->tm_mday;
  editMonth = timeinfo->tm_mon + 1;  // tm_mon is 0-11
  editYear = timeinfo->tm_year + 1900;
  editHour = timeinfo->tm_hour;
  editMinute = timeinfo->tm_min;
  editSecond = timeinfo->tm_sec;

  updateDateTime();
  updateWeather();
  lastUpdateTime = millis();
  requestUpdate();
}

void FeaturesActivity::onExit() {
  ActivityWithSubactivity::onExit();
}

static int daysInMonth(int year, int month) {
  static const int mdays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int d = mdays[(month - 1) % 12];
  // Leap year check
  if (month == 2) {
    bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
    if (leap) d = 29;
  }
  return d;
}

void FeaturesActivity::onUpdateAction() {
  // Clear any existing subactivity
  exitActivity();
  // Launch ReloadWiFiActivity as subactivity. When it finishes, destroy it and redraw ourself.
  enterNewActivity(new ReloadWiFiActivity(renderer, mappedInput, [this]() {
    exitActivity();
    requestUpdate();
  }));
}

void FeaturesActivity::loop() {
  if (subActivity) {
    subActivity->loop();
    return;
  }

  // Refresh wifi flag periodically
  wifiConnected = TimeManager::getInstance().isWiFiConnected();

  // Update date/time every UPDATE_INTERVAL (only for system time)
  unsigned long currentTimeMs = millis();
  if (currentTimeMs - lastUpdateTime >= UPDATE_INTERVAL) {
    // Only auto-update system time when WiFi connected
    if (wifiConnected) updateDateTime();
    updateWeather();
    lastUpdateTime = currentTimeMs;
    requestUpdate();
  }

  // Capture button presses once per loop to avoid double-processing
  const bool confirmPressed = mappedInput.wasPressed(MappedInputManager::Button::Confirm);
  const bool backPressed = mappedInput.wasPressed(MappedInputManager::Button::Back);

  // If WiFi is not connected, allow entering edit mode and adjustments
  if (!wifiConnected) {
    if (editing) {
      // While editing, Confirm advances sub-fields; if on last sub-field, exit edit mode and move to next item
      if (confirmPressed) {
        if (selectedMenu == FeatureMenu::DATE) {
          if (dateField == 2) {
            // finished editing date -> apply and exit edit mode
            TimeManager::getInstance().setManualDateTime(editYear, editMonth, editDay, editHour, editMinute, editSecond);
            editing = false;
            selectedMenu = static_cast<FeatureMenu>(ButtonNavigator::nextIndex(static_cast<int>(selectedMenu),
                                                                                  static_cast<int>(FeatureMenu::COUNT)));
          } else {
            dateField = dateField + 1;
          }
        } else if (selectedMenu == FeatureMenu::TIME) {
          if (timeField == 2) {
            // finished editing time -> apply and move to next main item
            // Apply full date+time together using setManualDateTime so date changes persist
            TimeManager::getInstance().setManualDateTime(editYear, editMonth, editDay, editHour, editMinute, editSecond);
            editing = false;
            // move to next main item
            selectedMenu = static_cast<FeatureMenu>(ButtonNavigator::nextIndex(static_cast<int>(selectedMenu),
                                                                                  static_cast<int>(FeatureMenu::COUNT)));
            // Update displayed system time immediately
            updateDateTime();
            lastUpdateTime = millis();
           } else {
            timeField = timeField + 1;
          }
        }
        requestUpdate();
      }

      // Up: increment selected sub-field
      buttonNavigator.onPressAndContinuous({MappedInputManager::Button::Up}, [this] {
        if (selectedMenu == FeatureMenu::DATE) {
          if (dateField == 0) {  // day
            int maxd = daysInMonth(editYear, editMonth);
            editDay = (editDay >= maxd) ? 1 : editDay + 1;
          } else if (dateField == 1) {  // month
            editMonth = (editMonth >= 12) ? 1 : editMonth + 1;
            int maxd = daysInMonth(editYear, editMonth);
            if (editDay > maxd) editDay = maxd;
          } else {  // year
            editYear = (editYear >= 9999) ? 2026 : (editYear + 1);
          }
        } else if (selectedMenu == FeatureMenu::TIME) {
          if (timeField == 0) {  // hour
            editHour = (editHour + 1) % 24;
          } else if (timeField == 1) {  // minute
            editMinute = (editMinute + 1) % 60;
          } else {  // second
            editSecond = (editSecond + 1) % 60;
          }
        }
        updateDateTime();
        requestUpdate();
      });

      // Down: decrement selected sub-field
      buttonNavigator.onPressAndContinuous({MappedInputManager::Button::Down}, [this] {
        if (selectedMenu == FeatureMenu::DATE) {
          if (dateField == 0) {  // day
            int maxd = daysInMonth(editYear, editMonth);
            editDay = (editDay <= 1) ? maxd : editDay - 1;
          } else if (dateField == 1) {  // month
            editMonth = (editMonth <= 1) ? 12 : editMonth - 1;
            int maxd = daysInMonth(editYear, editMonth);
            if (editDay > maxd) editDay = maxd;
          } else {  // year
            editYear = (editYear <= 2026) ? 2026 : (editYear - 1);
          }
        } else if (selectedMenu == FeatureMenu::TIME) {
          if (timeField == 0) {  // hour
            editHour = (editHour + 23) % 24;
          } else if (timeField == 1) {  // minute
            editMinute = (editMinute + 59) % 60;
          } else {  // second
            editSecond = (editSecond + 59) % 60;
          }
        }
        updateDateTime();
        requestUpdate();
      });

      // Back cancels editing
      if (backPressed) {
        editing = false;
        requestUpdate();
      }

    } else {  // not editing
      if (confirmPressed) {
        if (selectedMenu == FeatureMenu::UPDATE) {
          onUpdateAction();
          return;
        } else if (selectedMenu == FeatureMenu::DATE || selectedMenu == FeatureMenu::TIME) {
          editing = true;
          // Start editing at first sub-field
          dateField = 0;
          timeField = 0;
          requestUpdate();
        }
      }

      // Navigation between items using Up/Down
      buttonNavigator.onNext([this] {
        selectedMenu = static_cast<FeatureMenu>(ButtonNavigator::nextIndex(static_cast<int>(selectedMenu),
                                                                              static_cast<int>(FeatureMenu::COUNT)));
        requestUpdate();
      });

      buttonNavigator.onPrevious([this] {
        selectedMenu = static_cast<FeatureMenu>(ButtonNavigator::previousIndex(static_cast<int>(selectedMenu),
                                                                                  static_cast<int>(FeatureMenu::COUNT)));
        requestUpdate();
      });

      // Left/Right: cycle selected sub-field without entering edit mode (optional)
      buttonNavigator.onPressAndContinuous({MappedInputManager::Button::Left}, [this] {
        if (selectedMenu == FeatureMenu::DATE) {
          dateField = (dateField + 2) % 3;  // cycle backwards
        } else if (selectedMenu == FeatureMenu::TIME) {
          timeField = (timeField + 2) % 3;
        }
        requestUpdate();
      });

      buttonNavigator.onPressAndContinuous({MappedInputManager::Button::Right}, [this] {
        if (selectedMenu == FeatureMenu::DATE) {
          dateField = (dateField + 1) % 3;
        } else if (selectedMenu == FeatureMenu::TIME) {
          timeField = (timeField + 1) % 3;
        }
        requestUpdate();
      });

      // Up/Down when not editing: move selection handled by onNext/onPrevious above
    }

    // Back always exits activity (applies to both editing and non-editing states when offline)
    if (backPressed) {
      onBack();
      return;
    }
  } else {
    // WiFi is connected - UPDATE action available
    if (confirmPressed && selectedMenu == FeatureMenu::UPDATE) {
      onUpdateAction();
      return;
    }

    buttonNavigator.onNext([this] {
      selectedMenu = static_cast<FeatureMenu>(ButtonNavigator::nextIndex(static_cast<int>(selectedMenu),
                                                                            static_cast<int>(FeatureMenu::COUNT)));
      requestUpdate();
    });

    buttonNavigator.onPrevious([this] {
      selectedMenu = static_cast<FeatureMenu>(ButtonNavigator::previousIndex(static_cast<int>(selectedMenu),
                                                                                static_cast<int>(FeatureMenu::COUNT)));
      requestUpdate();
    });

    if (backPressed) {
      onBack();
      return;
    }
  }
}

void FeaturesActivity::render(Activity::RenderLock&& lock) {
  if (subActivity) {
    subActivity->render(std::move(lock));
    return;
  }

  renderer.clearScreen();

  auto metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  // Draw header
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_FEATURES));

  // Content area - display date, time, and weather info
  const int contentTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int contentHeight = pageHeight - contentTop - metrics.buttonHintsHeight - metrics.verticalSpacing * 2;

  // Create list of feature items - now with 6 items total
  std::vector<std::string> featureLabels = {
      std::string(tr(STR_RELOAD)), // Nút bấm
      tr(STR_FEATURES_DATE),
      tr(STR_FEATURES_TIME),
      tr(STR_FEATURES_AREA),
      tr(STR_FEATURES_WEATHER),
      tr(STR_FEATURES_TEMP)
  };

  std::vector<std::string> featureValues = {
      " ", // Trống cho đẹp, icon Reload nằm ở Label rồi
      currentDate,
      currentTime,
      weatherArea,
      weatherCondition,
      weatherTemperature
  };

  // Draw the features list
  GUI.drawList(
      renderer,
      Rect{0, contentTop, pageWidth, contentHeight},
      static_cast<int>(featureLabels.size()),
      static_cast<int>(selectedMenu),  // always selectable now for UPDATE
      [&featureLabels](int index) { return featureLabels[index]; },
      nullptr,
      nullptr,
      [&featureValues](int index) { return featureValues[index]; },
      true);  // highlight selected item

  // If editable, draw little indicator for which sub-field is selected
  if ((selectedMenu == FeatureMenu::DATE || selectedMenu == FeatureMenu::TIME)) {
    const char* hint = tr(STR_FEATURES_DATE_TIME_HINT_1);
    GUI.drawHelpText(renderer, Rect{metrics.contentSidePadding, pageHeight - metrics.buttonHintsHeight - 40, pageWidth, 20}, hint);

    const char* hint2 = tr(STR_FEATURES_DATE_TIME_HINT_2);
    GUI.drawHelpText(renderer, Rect{metrics.contentSidePadding, pageHeight - metrics.buttonHintsHeight - 20, pageWidth, 20}, hint2);
  }

  // Draw button hints
  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
