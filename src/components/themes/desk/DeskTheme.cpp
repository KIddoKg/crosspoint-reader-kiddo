#include "DeskTheme.h"

#include <Bitmap.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>

#include <algorithm>
#include <cstdio>
#include <string>

#include "CrossPointSettings.h"
#include "RecentBooksStore.h"
#include "activities/reader/FurthestProgress.h"
#include "components/DashboardWidgets.h"
#include "components/UITheme.h"
#include "components/icons/cover.h"
#include "fontIds.h"

namespace {
constexpr int cardPadding = 12;
constexpr int cardRadius = 10;
constexpr int blockGap = 10;
constexpr int bookCardHeight = 206;
constexpr int clockBlockHeight = 140;
constexpr int menuColumns = 4;
constexpr int menuIconSize = 32;

// Splits `text` into at most two lines that fit `maxWidth` (the second one truncated)
void drawTwoLineTitle(const GfxRenderer& renderer, const std::string& text, const int x, const int y,
                      const int maxWidth) {
  const int lineHeight = renderer.getLineHeight(UI_12_FONT_ID);
  if (renderer.getTextWidth(UI_12_FONT_ID, text.c_str(), EpdFontFamily::BOLD) <= maxWidth) {
    renderer.drawText(UI_12_FONT_ID, x, y, text.c_str(), true, EpdFontFamily::BOLD);
    return;
  }
  // Break at the last space that still fits on the first line
  size_t breakAt = std::string::npos;
  for (size_t pos = text.find(' '); pos != std::string::npos; pos = text.find(' ', pos + 1)) {
    if (renderer.getTextWidth(UI_12_FONT_ID, text.substr(0, pos).c_str(), EpdFontFamily::BOLD) > maxWidth) break;
    breakAt = pos;
  }
  if (breakAt == std::string::npos) {
    const auto line = renderer.truncatedText(UI_12_FONT_ID, text.c_str(), maxWidth, EpdFontFamily::BOLD);
    renderer.drawText(UI_12_FONT_ID, x, y, line.c_str(), true, EpdFontFamily::BOLD);
    return;
  }
  renderer.drawText(UI_12_FONT_ID, x, y, text.substr(0, breakAt).c_str(), true, EpdFontFamily::BOLD);
  const auto second = renderer.truncatedText(UI_12_FONT_ID, text.substr(breakAt + 1).c_str(), maxWidth,
                                             EpdFontFamily::BOLD);
  renderer.drawText(UI_12_FONT_ID, x, y + lineHeight, second.c_str(), true, EpdFontFamily::BOLD);
}

bool drawCoverImage(const GfxRenderer& renderer, const RecentBook& book, const int x, const int y, const int width,
                    const int height) {
  if (book.coverBmpPath.empty()) return false;
  const std::string coverBmpPath = UITheme::getCoverThumbPath(book.coverBmpPath, DeskMetrics::values.homeCoverHeight);
  HalFile file;
  if (!Storage.openFileForRead("DESK", coverBmpPath, file)) return false;
  Bitmap bitmap(file);
  if (bitmap.parseHeaders() != BmpReaderError::Ok) return false;
  const float ratio = static_cast<float>(bitmap.getWidth()) / static_cast<float>(bitmap.getHeight());
  const float slotRatio = static_cast<float>(width) / static_cast<float>(height);
  const float cropX = ratio > slotRatio ? 1.0f - (slotRatio / ratio) : 0.0f;
  renderer.drawBitmap(bitmap, x, y, width, height, cropX);
  return true;
}
}  // namespace

void DeskTheme::drawRecentBookCover(GfxRenderer& renderer, const Rect rect, const std::vector<RecentBook>& recentBooks,
                                    const int selectorIndex, bool& coverRendered, bool& coverBufferStored,
                                    bool& bufferRestored, std::function<bool()> storeCoverBuffer) const {
  const int pad = DeskMetrics::values.contentSidePadding;
  const Rect card{rect.x + pad, rect.y, rect.width - 2 * pad, bookCardHeight};
  const int coverHeight = DeskMetrics::values.homeCoverHeight;
  const int coverWidth = coverHeight * 2 / 3;
  const int coverX = card.x + cardPadding;
  const int coverY = card.y + (card.height - coverHeight) / 2;

  if (recentBooks.empty()) {
    renderer.drawRoundedRect(card.x, card.y, card.width, card.height, 1, cardRadius, true);
    drawEmptyRecents(renderer, card);
  } else {
    const RecentBook& book = recentBooks[0];

    // The cover is decoded once and kept in the stored frame buffer; everything else is redrawn on top
    if (!coverRendered) {
      // Clear the slot first: a restored buffer may still hold the placeholder drawn before the thumbnail existed,
      // and a 1-bit cover only paints its black pixels
      renderer.fillRect(coverX, coverY, coverWidth, coverHeight, false);
      if (!drawCoverImage(renderer, book, coverX, coverY, coverWidth, coverHeight)) {
        renderer.fillRect(coverX, coverY + coverHeight / 3, coverWidth, 2 * coverHeight / 3, true);
        renderer.drawIcon(CoverIcon, coverX + (coverWidth - 32) / 2, coverY + 24, 32, 32);
      }
      coverBufferStored = storeCoverBuffer();
      coverRendered = true;
    }
    renderer.drawRect(coverX, coverY, coverWidth, coverHeight, true);

    const bool selected = selectorIndex == 0;
    renderer.drawRoundedRect(card.x, card.y, card.width, card.height, selected ? 3 : 1, cardRadius, true);

    const int textX = coverX + coverWidth + cardPadding;
    const int textWidth = card.x + card.width - cardPadding - textX;
    int y = card.y + cardPadding + 4;
    drawTwoLineTitle(renderer, book.title, textX, y, textWidth);
    y += renderer.getLineHeight(UI_12_FONT_ID) * 2 + 4;
    if (!book.author.empty()) {
      const auto author = renderer.truncatedText(UI_10_FONT_ID, book.author.c_str(), textWidth);
      renderer.drawText(UI_10_FONT_ID, textX, y, author.c_str(), true);
    }

    // Progress: filled to the furthest point read, tick at the current position, "Finished" past the end
    const uint8_t furthest = std::max(book.furthest, book.progress);
    const int barY = card.y + card.height - cardPadding - 70;
    char percentText[32];
    if (furthest >= FurthestProgress::FINISHED_PERCENT) {
      snprintf(percentText, sizeof(percentText), "%s", tr(STR_BOOK_FINISHED));
    } else {
      snprintf(percentText, sizeof(percentText), "%u%%", static_cast<unsigned>(furthest));
    }
    renderer.drawText(UI_12_FONT_ID, textX, barY - renderer.getLineHeight(UI_12_FONT_ID) - 4, percentText, true,
                      EpdFontFamily::BOLD);
    constexpr int barHeight = 10;
    renderer.drawRect(textX, barY, textWidth, barHeight);
    const int filled = textWidth * furthest / 100;
    if (filled > 2) renderer.fillRect(textX + 1, barY + 1, filled - 2, barHeight - 2);
    if (book.progress < furthest) {
      const int markerX = std::max(textX + 1, textX + textWidth * book.progress / 100 - 1);
      renderer.fillRect(markerX, barY + 1, 2, barHeight - 2, false);
      renderer.fillRect(markerX, barY - 3, 2, 3);
      renderer.fillRect(markerX, barY + barHeight, 2, 3);
    }

    const int minutesLeft = DashboardWidgets::chapterMinutesLeft(book);
    if (minutesLeft >= 0) {
      char left[48];
      snprintf(left, sizeof(left), tr(STR_MINUTES_LEFT_CHAPTER), minutesLeft);
      const auto fitted = renderer.truncatedText(UI_10_FONT_ID, left, textWidth);
      renderer.drawText(UI_10_FONT_ID, textX, barY + barHeight + 10, fitted.c_str(), true);
    }
  }

  // Clock / date / weather block
  const Rect clockRect{card.x, card.y + card.height + blockGap, card.width, clockBlockHeight};
  renderer.drawRoundedRect(clockRect.x, clockRect.y, clockRect.width, clockRect.height, 1, cardRadius, true);
  DashboardWidgets::drawClock(renderer, clockRect);

  // Reading goal block (fills the rest of the tile)
  const int goalY = clockRect.y + clockRect.height + blockGap;
  const Rect goalRect{card.x, goalY, card.width, rect.y + rect.height - goalY};
  if (SETTINGS.readingStatsEnabled && goalRect.height > 30) {
    renderer.drawRoundedRect(goalRect.x, goalRect.y, goalRect.width, goalRect.height, 1, cardRadius, true);
    DashboardWidgets::drawGoal(renderer, goalRect);
  }
}

void DeskTheme::drawButtonMenu(GfxRenderer& renderer, const Rect rect, const int buttonCount, const int selectedIndex,
                               const std::function<std::string(int index)>& buttonLabel,
                               const std::function<UIIcon(int index)>& rowIcon) const {
  const auto& m = DeskMetrics::values;
  const int tileWidth = (rect.width - 2 * m.contentSidePadding - (menuColumns - 1) * m.menuSpacing) / menuColumns;
  const int tileHeight = m.menuRowHeight;

  for (int i = 0; i < buttonCount; ++i) {
    const int tileX = rect.x + m.contentSidePadding + (i % menuColumns) * (tileWidth + m.menuSpacing);
    const int tileY = rect.y + (i / menuColumns) * (tileHeight + m.menuSpacing);
    const bool selected = selectedIndex == i;

    if (selected) {
      renderer.fillRoundedRect(tileX, tileY, tileWidth, tileHeight, cardRadius, Color::Black);
    } else {
      renderer.drawRoundedRect(tileX, tileY, tileWidth, tileHeight, 1, cardRadius, true);
    }

    if (rowIcon != nullptr) {
      if (const uint8_t* icon = menuIcon(rowIcon(i))) {
        const int iconX = tileX + (tileWidth - menuIconSize) / 2;
        const int iconY = tileY + 10;
        if (selected) {
          // Icons are drawn black-on-white: punch a white backdrop into the selected (black) tile
          renderer.fillRoundedRect(iconX - 4, iconY - 4, menuIconSize + 8, menuIconSize + 8, 6, Color::White);
        }
        renderer.drawIcon(icon, iconX, iconY, menuIconSize, menuIconSize);
      }
    }

    const auto label = renderer.truncatedText(UI_10_FONT_ID, buttonLabel(i).c_str(), tileWidth - 8);
    const int labelWidth = renderer.getTextWidth(UI_10_FONT_ID, label.c_str());
    renderer.drawText(UI_10_FONT_ID, tileX + (tileWidth - labelWidth) / 2,
                      tileY + tileHeight - renderer.getLineHeight(UI_10_FONT_ID) - 8, label.c_str(), !selected);
  }
}
