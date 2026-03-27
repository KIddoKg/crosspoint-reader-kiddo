#include "StatusBar.h"
#include <HalDisplay.h>
#include <Logging.h>
#include <cstdio>
#include <cstring>

void StatusBar::render() {
  // Draw background bar
  int screenWidth = renderer.getScreenWidth();
  renderer.fillRect(0, y, screenWidth, height, false);  // false = white

  // Draw border
  renderer.drawRect(0, y, screenWidth, height, true);  // true = black

  // Left: Time (HH:MM)
  std::string timeStr = getTimeString();
  const int fontId = 0;
  renderer.drawText(fontId, 5, y + 5, timeStr.c_str(), true, EpdFontFamily::REGULAR);

  // Middle: Temperature (with WiFi indicator)
  std::string tempStr = getTemperatureString();
  int centerX = screenWidth / 2 - 30;
  renderer.drawText(fontId, centerX, y + 5, tempStr.c_str(), true, EpdFontFamily::REGULAR);

  // Right: Battery indicator
  renderBattery(screenWidth - 60, y + 4, batteryPercent);
}

std::string StatusBar::getTimeString() const {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%H:%M", timeinfo);
  return std::string(buffer);
}

std::string StatusBar::getTemperatureString() const {
  char buffer[30];
  if (hasWiFi) {
    snprintf(buffer, sizeof(buffer), "%.0f°C %s", currentTemp, "📡");
  } else {
    snprintf(buffer, sizeof(buffer), "%.0f°C", currentTemp);
  }
  return std::string(buffer);
}

void StatusBar::renderBattery(int x, int y, int percent) const {
  // Draw battery outline: rectangular shape
  // Body: 30x12px, terminal: 3x6px
  renderer.drawRect(x, y, 28, 12, true);  // true = black

  // Draw terminal (right side)
  renderer.fillRect(x + 28, y + 3, 2, 6, true);  // true = black

  // Draw battery fill based on percentage
  if (percent > 0) {
    int fillWidth = (26 * percent) / 100;
    if (fillWidth > 0) {
      // Different colors based on battery level
      bool fillBlack = true;  // Black fill
      if (percent < 20) {
        fillBlack = true;  // Low battery - black
      }
      renderer.fillRect(x + 1, y + 1, fillWidth, 10, fillBlack);
    }
  }

  // Draw percentage text below
  char percentStr[6];
  snprintf(percentStr, sizeof(percentStr), "%d%%", percent);
  const int fontId = 0;
  renderer.drawText(fontId, x - 15, y + 1, percentStr, true, EpdFontFamily::REGULAR);
}
