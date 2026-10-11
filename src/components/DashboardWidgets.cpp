#include "DashboardWidgets.h"

#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <string>

#include "CrossPointSettings.h"
#include "ReadingStatsStore.h"
#include "RecentBooksStore.h"
#include "fontIds.h"
#include "util/LunarCalendar.h"
#include "util/StringUtils.h"
#include "util/TimeManager.h"
#include "util/WeatherManager.h"

namespace {
#ifndef OMIT_FONTS
constexpr int CLOCK_FONT_ID = NOTOSANS_18_FONT_ID;
#else
constexpr int CLOCK_FONT_ID = UI_12_FONT_ID;
#endif

void drawCentered(const GfxRenderer& renderer, const int fontId, const Rect& rect, const int y, const char* text,
                  const EpdFontFamily::Style style = EpdFontFamily::REGULAR) {
  const auto fitted = renderer.truncatedText(fontId, text, rect.width - 8, style);
  const int width = renderer.getTextWidth(fontId, fitted.c_str(), style);
  renderer.drawText(fontId, rect.x + (rect.width - width) / 2, y, fitted.c_str(), true, style);
}

// "City\n+28°C Cloudy" -> "+28°C Cloudy - City"
std::string weatherLine() {
  if (!SETTINGS.weatherEnabled) return "";
  const auto& weather = WeatherManager::getInstance();
  if (!weather.hasRealData()) return tr(STR_NO_WEATHER);
  const std::string data = weather.getWeatherData();
  const size_t newline = data.find('\n');
  if (newline == std::string::npos) return data;
  std::string conditions = data.substr(newline + 1);
  conditions.erase(0, conditions.find_first_not_of(' '));
  return conditions + " - " + data.substr(0, newline);
}
}  // namespace

namespace DashboardWidgets {

void drawClock(const GfxRenderer& renderer, const Rect rect, const bool dateOnly) {
  const bool clockValid = TimeManager::getInstance().hasValidClock();
  char large[16] = "--:--";
  char date[64] = "";
  if (clockValid) {
    const time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    const auto lunar = LunarCalendar::fromSolar(t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
    if (dateOnly) {
      snprintf(large, sizeof(large), "%02d/%02d", t.tm_mday, t.tm_mon + 1);
      snprintf(date, sizeof(date), "%04d - %s %d/%d%s", t.tm_year + 1900, tr(STR_LUNAR_SHORT), lunar.day,
               lunar.month, lunar.leapMonth ? "*" : "");
    } else {
      TimeManager::getInstance().formatClock(large, sizeof(large));
      snprintf(date, sizeof(date), "%02d/%02d/%04d - %s %d/%d%s", t.tm_mday, t.tm_mon + 1, t.tm_year + 1900,
               tr(STR_LUNAR_SHORT), lunar.day, lunar.month, lunar.leapMonth ? "*" : "");
    }
  } else if (dateOnly) {
    large[0] = '\0';
  }

  const int clockHeight = renderer.getLineHeight(CLOCK_FONT_ID);
  const int smallHeight = renderer.getLineHeight(UI_10_FONT_ID);
  const std::string weather = weatherLine();
  const int contentHeight = clockHeight + smallHeight * (weather.empty() ? 1 : 2) + 8;
  int y = rect.y + std::max(0, (rect.height - contentHeight) / 2);

  if (large[0]) drawCentered(renderer, CLOCK_FONT_ID, rect, y, large, EpdFontFamily::BOLD);
  y += clockHeight + 4;
  if (date[0]) drawCentered(renderer, UI_10_FONT_ID, rect, y, date);
  y += smallHeight + 4;
  if (!weather.empty()) drawCentered(renderer, UI_10_FONT_ID, rect, y, weather.c_str());
}

void drawGoal(const GfxRenderer& renderer, const Rect rect) {
  constexpr int padding = 10;
  constexpr int barHeight = 10;
  const unsigned today = READING_STATS.getTodayMinutes();
  const unsigned goal = SETTINGS.dailyGoalMinutes;

  char left[48];
  if (goal > 0) {
    snprintf(left, sizeof(left), tr(STR_TODAY_GOAL), today, goal);
  } else {
    snprintf(left, sizeof(left), tr(STR_TODAY_MINUTES), today);
  }
  char right[24];
  snprintf(right, sizeof(right), tr(STR_STREAK_SHORT), static_cast<unsigned>(READING_STATS.getStreak()));

  const int textY = rect.y + padding;
  renderer.drawText(UI_10_FONT_ID, rect.x + padding, textY, left, true);
  const int rightWidth = renderer.getTextWidth(UI_10_FONT_ID, right);
  renderer.drawText(UI_10_FONT_ID, rect.x + rect.width - padding - rightWidth, textY, right, true);

  if (goal > 0) {
    const int barY = textY + renderer.getLineHeight(UI_10_FONT_ID) + 6;
    const int barWidth = rect.width - 2 * padding;
    renderer.drawRect(rect.x + padding, barY, barWidth, barHeight);
    const int filled = static_cast<int>(barWidth * std::min(today, goal) / goal);
    if (filled > 2) {
      renderer.fillRect(rect.x + padding + 1, barY + 1, filled - 2, barHeight - 2);
    }
  }
}

int chapterMinutesLeft(const RecentBook& book) {
  if (!StringUtils::checkFileExtension(book.path, ".epub") || book.coverBmpPath.empty()) return -1;
  if (READING_STATS.getAvgSecondsPerPage() == 0) return -1;

  // The cover thumbnail lives in the book's cache directory, next to progress.bin
  const std::string cacheDir = book.coverBmpPath.substr(0, book.coverBmpPath.rfind('/'));
  HalFile f;
  if (!Storage.openFileForRead("DASH", cacheDir + "/progress.bin", f)) return -1;
  uint8_t data[6];
  const int read = f.read(data, sizeof(data));
  if (read != sizeof(data)) return -1;
  const int page = data[2] | (data[3] << 8);
  const int pageCount = data[4] | (data[5] << 8);
  if (pageCount <= 0 || page >= pageCount) return -1;
  return READING_STATS.estimateMinutes(static_cast<uint32_t>(pageCount - page - 1));
}

}  // namespace DashboardWidgets
