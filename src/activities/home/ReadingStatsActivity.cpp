#include "ReadingStatsActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <Bitmap.h>
#include <HalStorage.h>

#include "ReadingStatsStore.h"
#include "components/UITheme.h"
#include "fontIds.h"

void ReadingStatsActivity::onEnter() {
  Activity::onEnter();
  READING_STATS.loadFromFile();
  requestUpdate();
}

void ReadingStatsActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    onGoBack();
  }
}

void ReadingStatsActivity::render(Activity::RenderLock&&) {
  renderer.clearScreen();

  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();
  auto metrics = UITheme::getInstance().getMetrics();

  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_READING_STATS));

  const int contentTop = metrics.topPadding + metrics.headerHeight + 20;
  const int sideMargin = metrics.contentSidePadding;
  const int totalWidth = pageWidth - 2 * sideMargin;
  
  // Layout parameters
  const int leftWidth = totalWidth * 62 / 100;
  const int rightWidth = totalWidth - leftWidth - 10; // 10px spacing
  const int statRowHeight = 75;
  const int statGap = 12; // Gap between Today and Total boxes
  const int gridHeight = statRowHeight * 2 + statGap;

  // 1. Today's Reading (Left Top)
  Rect todayRect(sideMargin, contentTop, leftWidth, statRowHeight);
  renderer.drawRoundedRect(todayRect.x, todayRect.y, todayRect.width, todayRect.height, 1, 8, true);
  renderer.drawText(UI_10_FONT_ID, todayRect.x + 8, todayRect.y + 10, tr(STR_TODAY_READING));
  
  auto formatFullDuration = [](char* buf, size_t size, uint32_t totalMinutes) {
      uint32_t years = totalMinutes / 525600;
      uint32_t rem = totalMinutes % 525600;
      uint32_t days = rem / 1440;
      rem %= 1440;
      uint32_t hours = rem / 60;
      uint32_t mins = rem % 60;

      int written = 0;
      if (years > 0) {
          written += snprintf(buf + written, size - written, tr(STR_YEARS_SHORT), years);
          if (written < size - 1) buf[written++] = ' ';
      }
      if (days > 0) {
          written += snprintf(buf + written, size - written, tr(STR_DAYS_SHORT), days);
          if (written < size - 1) buf[written++] = ' ';
      }
      if (hours > 0 || years > 0 || days > 0) {
          // Use STR_HOURS_MINUTES for the last part if we have hours/mins
          snprintf(buf + written, size - written, tr(STR_HOURS_MINUTES), hours, mins);
      } else {
          // Just minutes
          snprintf(buf + written, size - written, tr(STR_MINUTES_SHORT), mins);
      }
  };

  char buf[128];
  uint32_t todayMin = READING_STATS.getTodayMinutes();
  formatFullDuration(buf, sizeof(buf), todayMin);
  renderer.drawText(UI_12_FONT_ID, todayRect.x + 15, todayRect.y + 40, buf);

  // 2. Total Reading Time (Left Bottom)
  Rect totalTimeRect(sideMargin, contentTop + statRowHeight + statGap, leftWidth, statRowHeight);
  renderer.drawRoundedRect(totalTimeRect.x, totalTimeRect.y, totalTimeRect.width, totalTimeRect.height, 1, 8, true);
  renderer.drawText(UI_10_FONT_ID, totalTimeRect.x + 8, totalTimeRect.y + 10, tr(STR_TOTAL_READING));
  
  uint32_t totalMin = READING_STATS.getTotalMinutes();
  formatFullDuration(buf, sizeof(buf), totalMin);
  renderer.drawText(UI_12_FONT_ID, totalTimeRect.x + 15, totalTimeRect.y + 40, buf);

  // 3. Streak (Right - spanning 2 rows + gap)
  Rect streakRect(sideMargin + leftWidth + 10, contentTop, rightWidth, gridHeight);
  renderer.drawRoundedRect(streakRect.x, streakRect.y, streakRect.width, streakRect.height, 1, 8, true);
  renderer.drawText(UI_10_FONT_ID, streakRect.x + 8, streakRect.y + 10, tr(STR_NUM_STREAK));
  
  uint32_t streak = READING_STATS.getStreak();
  snprintf(buf, sizeof(buf), tr(STR_STREAK_DAYS), streak);
  // Center large text in the large block (vertically center in the space below the title)
  int textWidth = renderer.getTextWidth(UI_12_FONT_ID, buf);
  int textHeight = renderer.getLineHeight(UI_12_FONT_ID);
  int streakTextY = streakRect.y + 35 + (gridHeight - 35 - textHeight) / 2;
  renderer.drawText(UI_12_FONT_ID, streakRect.x + (rightWidth - textWidth) / 2, streakTextY, buf);

  // 4. Most Read Books
  const int booksTop = contentTop + gridHeight + 55; // Increased margin from boxes
  renderer.drawText(UI_12_FONT_ID, sideMargin, booksTop, tr(STR_MOST_READ_BOOKS));
  renderer.drawLine(sideMargin, booksTop + 35, pageWidth - sideMargin, booksTop + 35); // Lowered line further

  auto topBooks = READING_STATS.getTopBooks(10);
  int itemY = booksTop + 55; // Increased start Y for items
  if (topBooks.empty()) {
      renderer.drawText(UI_10_FONT_ID, sideMargin + 10, itemY, tr(STR_NO_ENTRIES));
  } else {
      for (size_t i = 0; i < topBooks.size(); ++i) {
          // Index
          snprintf(buf, sizeof(buf), "%d.", (int)i + 1);
          renderer.drawText(UI_10_FONT_ID, sideMargin, itemY, buf);
          
          // Time string (far right)
          char timeBuf[16];
          if (topBooks[i].totalMinutes >= 60) {
              snprintf(timeBuf, sizeof(timeBuf), tr(STR_HOURS_MINUTES), topBooks[i].totalMinutes / 60, topBooks[i].totalMinutes % 60);
          } else {
              snprintf(timeBuf, sizeof(timeBuf), tr(STR_MINUTES_SHORT), topBooks[i].totalMinutes);
          }
          int timeWidth = renderer.getTextWidth(UI_10_FONT_ID, timeBuf);
          int timeX = pageWidth - sideMargin - timeWidth;
          renderer.drawText(UI_10_FONT_ID, timeX, itemY, timeBuf);
          
          // Progress string (to the left of time)
          char progBuf[16];
          snprintf(progBuf, sizeof(progBuf), "%d%%", topBooks[i].progress);
          int progWidth = renderer.getTextWidth(UI_10_FONT_ID, progBuf);
          int progX = timeX - progWidth - 15; // 15px gap
          renderer.drawText(UI_10_FONT_ID, progX, itemY, progBuf);

          // Title (truncated to fit remaining space)
          int maxTitleWidth = progX - (sideMargin + 25) - 10;
          auto title = renderer.truncatedText(UI_10_FONT_ID, topBooks[i].title.c_str(), maxTitleWidth);
          renderer.drawText(UI_10_FONT_ID, sideMargin + 25, itemY, title.c_str());
          
          itemY += 32;
          if (i < topBooks.size() - 1 && itemY < pageHeight - 50) {
              renderer.drawLine(sideMargin + 25, itemY - 6, pageWidth - sideMargin, itemY - 6, false);
          } else if (itemY >= pageHeight - 50) {
              break; 
          }
      }
  }

  // Help text
  const auto labels = mappedInput.mapLabels(tr(STR_HOME), "", "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
