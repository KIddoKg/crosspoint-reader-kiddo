#include "GridHomeTheme.h"

#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>
#include <ctime>

#include "RecentBooksStore.h"
#include "ReadingStatsStore.h"
#include "components/UITheme.h"
#include "components/icons/book.h"
#include "components/icons/cover.h"
#include "components/icons/file24.h"
#include "components/icons/folder.h"
#include "components/icons/folder24.h"
#include "components/icons/image24.h"
#include "components/icons/library.h"
#include "components/icons/recent.h"
#include "components/icons/reload.h"
#include "components/icons/settings2.h"
#include "components/icons/text24.h"
#include "components/icons/transfer.h"
#include "components/icons/wifi.h"
#include "fontIds.h"
#include "activities/reader/FurthestProgress.h"
#include "util/TimeManager.h"

namespace {
constexpr int cornerRadius = 16;
constexpr int tilePadding = 16;
constexpr int mainMenuIconSize = 32;
constexpr int mainMenuColumns = 2;
constexpr int hPaddingInSelection = 8;
constexpr int bookCornerRadius = 8;

const uint8_t* iconForName(UIIcon icon) {
  switch (icon) {
    case UIIcon::Folder: return FolderIcon;
    case UIIcon::Book: return BookIcon;
    case UIIcon::File: return File24Icon;
    case UIIcon::Recent: return RecentIcon;
    case UIIcon::Settings: return Settings2Icon;
    case UIIcon::Transfer: return TransferIcon;
    case UIIcon::Library: return LibraryIcon;
    case UIIcon::Reload: return ReloadIcon;
    case UIIcon::Wifi: return WifiIcon;
    default: return nullptr;
  }
}
}  // namespace

const uint8_t* GridHomeTheme::menuIcon(const UIIcon icon) { return iconForName(icon); }

void GridHomeTheme::drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                                        const int selectorIndex, bool& coverRendered, bool& coverBufferStored,
                                        bool& bufferRestored, std::function<bool()> storeCoverBuffer) const {
  const int maxBooks = GridHomeMetrics::values.homeRecentBooksCount;
  const int tileWidth = (rect.width - 2 * GridHomeMetrics::values.contentSidePadding) / maxBooks;
  const int tileHeight = rect.height;
  const int bookTitleHeight = tileHeight - GridHomeMetrics::values.homeCoverHeight - hPaddingInSelection;
  const int tileY = rect.y;
  const bool hasContinueReading = !recentBooks.empty();

  if (hasContinueReading) {
    if (!coverRendered) {
      for (int i = 0; i < std::min(static_cast<int>(recentBooks.size()), maxBooks); i++) {
        std::string coverPath = recentBooks[i].coverBmpPath;
        bool hasCover = !coverPath.empty();
        int tileX = GridHomeMetrics::values.contentSidePadding + tileWidth * i;

        if (hasCover) {
          const std::string coverBmpPath = UITheme::getCoverThumbPath(coverPath, GridHomeMetrics::values.homeCoverHeight);
          FsFile file;
          if (Storage.openFileForRead("HOME", coverBmpPath, file)) {
            Bitmap bitmap(file);
            if (bitmap.parseHeaders() == BmpReaderError::Ok) {
              float cHeight = static_cast<float>(bitmap.getHeight());
              float cWidth = static_cast<float>(bitmap.getWidth());
              float ratio = cWidth / cHeight;
              const float tileRatio = static_cast<float>(tileWidth - 2 * hPaddingInSelection) /
                                      static_cast<float>(GridHomeMetrics::values.homeCoverHeight);
              float cropX = 1.0f - (tileRatio / ratio);
              renderer.drawBitmap(bitmap, tileX + hPaddingInSelection, tileY + hPaddingInSelection,
                                  tileWidth - 2 * hPaddingInSelection, GridHomeMetrics::values.homeCoverHeight, cropX);
            } else {
              hasCover = false;
            }
            file.close();
          } else {
            hasCover = false;
          }
        }

        renderer.drawRect(tileX + hPaddingInSelection, tileY + hPaddingInSelection, tileWidth - 2 * hPaddingInSelection,
                          GridHomeMetrics::values.homeCoverHeight, true);

        if (!hasCover) {
          renderer.fillRect(tileX + hPaddingInSelection,
                            tileY + hPaddingInSelection + (GridHomeMetrics::values.homeCoverHeight / 3),
                            tileWidth - 2 * hPaddingInSelection, 2 * GridHomeMetrics::values.homeCoverHeight / 3, true);
          renderer.drawIcon(CoverIcon, tileX + hPaddingInSelection + (tileWidth - 2 * hPaddingInSelection - 32) / 2,
                            tileY + hPaddingInSelection + 24, 32, 32);
        }
      }
      coverBufferStored = storeCoverBuffer();
      coverRendered = true;
    }

    for (int i = 0; i < std::min(static_cast<int>(recentBooks.size()), maxBooks); i++) {
      bool bookSelected = (selectorIndex == i);
      int tileX = GridHomeMetrics::values.contentSidePadding + tileWidth * i;
      auto title = renderer.truncatedText(UI_10_FONT_ID, recentBooks[i].title.c_str(), tileWidth - 2 * hPaddingInSelection);

      if (bookSelected) {
        renderer.fillRoundedRect(tileX, tileY, tileWidth, hPaddingInSelection, bookCornerRadius, true, true, false, false, Color::LightGray);
        renderer.fillRectDither(tileX, tileY + hPaddingInSelection, hPaddingInSelection, GridHomeMetrics::values.homeCoverHeight, Color::LightGray);
        renderer.fillRectDither(tileX + tileWidth - hPaddingInSelection, tileY + hPaddingInSelection, hPaddingInSelection, GridHomeMetrics::values.homeCoverHeight, Color::LightGray);
        renderer.fillRoundedRect(tileX, tileY + GridHomeMetrics::values.homeCoverHeight + hPaddingInSelection, tileWidth, bookTitleHeight, bookCornerRadius, false, false, true, true, Color::LightGray);
      }

      renderer.drawText(UI_10_FONT_ID, tileX + hPaddingInSelection,
                        tileY + tileHeight - bookTitleHeight + hPaddingInSelection + 5, title.c_str(), true);
    }
  } else {
    drawEmptyRecents(renderer, rect);
  }
}

void GridHomeTheme::drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                                   const std::function<std::string(int index)>& buttonLabel,
                                   const std::function<UIIcon(int index)>& rowIcon) const {
  const int tileWidth = (rect.width - GridHomeMetrics::values.contentSidePadding * 2 - GridHomeMetrics::values.menuSpacing) / mainMenuColumns;
  const int tileHeight = GridHomeMetrics::values.menuRowHeight;

  for (int i = 0; i < buttonCount; ++i) {
    const int col = i % mainMenuColumns;
    const int row = i / mainMenuColumns;
    const int tileX = rect.x + GridHomeMetrics::values.contentSidePadding + col * (tileWidth + GridHomeMetrics::values.menuSpacing);
    const int tileY = rect.y + GridHomeMetrics::values.verticalSpacing + row * (tileHeight + GridHomeMetrics::values.menuSpacing);

    const bool selected = (selectedIndex == i);

    if (selected) {
      renderer.fillRoundedRect(tileX, tileY, tileWidth, tileHeight, cornerRadius, Color::LightGray);
    } else {
      const int borderThickness = 2;
      renderer.fillRoundedRect(tileX - borderThickness, tileY - borderThickness, tileWidth + borderThickness * 2,
                               tileHeight + borderThickness * 2, cornerRadius + borderThickness, Color::Black);
      renderer.fillRoundedRect(tileX, tileY, tileWidth, tileHeight, cornerRadius, Color::White);
    }

    if (rowIcon != nullptr) {
      UIIcon icon = rowIcon(i);
      const uint8_t* iconBitmap = iconForName(icon);
      if (iconBitmap != nullptr) {
        renderer.drawIcon(iconBitmap, tileX + (tileWidth - mainMenuIconSize) / 2, tileY + tilePadding, mainMenuIconSize, mainMenuIconSize);
      }
    }

    std::string labelStr = buttonLabel(i);
    const int tWidth = renderer.getTextWidth(UI_12_FONT_ID, labelStr.c_str());
    renderer.drawText(UI_12_FONT_ID, tileX + (tileWidth - tWidth) / 2, tileY + tileHeight - 20 - tilePadding, labelStr.c_str(), true);
  }
}

// Grid-based recent books list: 3 columns with cover image + title + progress bar
void GridHomeTheme::drawRecentBooksList(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                                        int selectorIndex) const {
  constexpr int columns = 3;
  constexpr int cellPadding = 8;
  constexpr int progressBarHeight = 4;
  constexpr int titleAreaHeight = 36;  // Space for title text below cover
  constexpr int progressAreaHeight = progressBarHeight + 20;  // Progress bar + time text + spacing
  constexpr int cellSpacingX = 4;

  const int totalWidth = rect.width - 2 * GridHomeMetrics::values.contentSidePadding;
  const int cellWidth = (totalWidth - (columns - 1) * cellSpacingX) / columns;

  // Cover height = cell height - title - progress - padding
  const int coverHeight = GridHomeMetrics::values.homeCoverHeight;
  const int cellHeight = coverHeight + titleAreaHeight + progressAreaHeight + cellPadding * 2;
  const int rowsPerPage = rect.height / cellHeight;
  const int itemsPerPage = rowsPerPage * columns;

  // Calculate page based on selector
  const int pageStartIndex = (selectorIndex / itemsPerPage) * itemsPerPage;
  const int itemCount = static_cast<int>(recentBooks.size());

  for (int i = pageStartIndex; i < itemCount && i < pageStartIndex + itemsPerPage; i++) {
    const int localIndex = i - pageStartIndex;
    const int col = localIndex % columns;
    const int row = localIndex / columns;

    const int cellX = rect.x + GridHomeMetrics::values.contentSidePadding + col * (cellWidth + cellSpacingX);
    const int cellY = rect.y + row * cellHeight;
    const bool selected = (i == selectorIndex);

    // Draw selection highlight
    if (selected) {
      renderer.fillRoundedRect(cellX, cellY, cellWidth, cellHeight, bookCornerRadius, Color::LightGray);
    }

    // Draw cover image
    const int coverX = cellX + cellPadding;
    const int coverY = cellY + cellPadding;
    const int coverWidth = cellWidth - 2 * cellPadding;
    bool hasCover = false;

    if (!recentBooks[i].coverBmpPath.empty()) {
      const std::string coverBmpPath = UITheme::getCoverThumbPath(recentBooks[i].coverBmpPath, coverHeight);
      FsFile file;
      if (Storage.openFileForRead("RBG", coverBmpPath, file)) {
        Bitmap bitmap(file);
        if (bitmap.parseHeaders() == BmpReaderError::Ok) {
          hasCover = true;
          float cHeight = static_cast<float>(bitmap.getHeight());
          float cWidth = static_cast<float>(bitmap.getWidth());
          float ratio = cWidth / cHeight;
          const float tileRatio = static_cast<float>(coverWidth) / static_cast<float>(coverHeight);
          float cropX = 1.0f - (tileRatio / ratio);
          renderer.drawBitmap(bitmap, coverX, coverY, coverWidth, coverHeight, cropX);
        }
        file.close();
      }
    }

    // Draw cover border
    renderer.drawRect(coverX, coverY, coverWidth, coverHeight, true);

    // Draw placeholder if no cover
    if (!hasCover) {
      renderer.fillRect(coverX, coverY + (coverHeight / 3), coverWidth, 2 * coverHeight / 3, true);
      renderer.drawIcon(CoverIcon, coverX + (coverWidth - 32) / 2, coverY + 24, 32, 32);
    }

    // Draw title below cover
    const int titleY = coverY + coverHeight + 4;
    auto title = renderer.truncatedText(UI_10_FONT_ID, recentBooks[i].title.c_str(), coverWidth);
    renderer.drawText(UI_10_FONT_ID, coverX, titleY, title.c_str(), true);

    // Draw progress bar below title: filled up to the furthest point read, with a tick at the current position
    // when the reader went back (e.g. re-reading an earlier chapter)
    const int progressY = titleY + titleAreaHeight - 4;
    const int progressWidth = coverWidth;
    const uint8_t furthest = std::max(recentBooks[i].furthest, recentBooks[i].progress);
    const int filledWidth = progressWidth * furthest / 100;

    // Progress bar outline
    renderer.drawRect(coverX, progressY, progressWidth, progressBarHeight);
    // Progress bar fill
    if (filledWidth > 0) {
      renderer.fillRect(coverX + 1, progressY + 1, filledWidth - 2, progressBarHeight - 2);
    }
    if (recentBooks[i].progress < furthest) {
      const int markerX = std::max(coverX + 1, coverX + progressWidth * recentBooks[i].progress / 100 - 1);
      renderer.fillRect(markerX, progressY + 1, 2, progressBarHeight - 2, false);
      renderer.fillRect(markerX, progressY - 3, 2, 3);
      renderer.fillRect(markerX, progressY + progressBarHeight, 2, 3);
    }
    if (furthest >= FurthestProgress::FINISHED_PERCENT) {
      renderer.drawText(UI_10_FONT_ID, coverX, progressY + progressBarHeight + 4, tr(STR_BOOK_FINISHED), true,
                        EpdFontFamily::BOLD);
    }

    if (SETTINGS.readingStatsEnabled) {
      uint32_t mins = READING_STATS.getBookMinutes(recentBooks[i].path);
      char timeBuf[32];
      if (mins >= 1440) { // More than 24h: show "Xd Yh" or just "Xh"
        snprintf(timeBuf, sizeof(timeBuf), "%dh", (int)(mins / 60));
      } else if (mins >= 60) {
        snprintf(timeBuf, sizeof(timeBuf), tr(STR_HOURS_MINUTES), mins / 60, mins % 60);
      } else {
        snprintf(timeBuf, sizeof(timeBuf), tr(STR_MINUTES_VSHORT), mins);
      }
      int timeWidth = renderer.getTextWidth(UI_10_FONT_ID, timeBuf);
      renderer.drawText(UI_10_FONT_ID, coverX + progressWidth - timeWidth, progressY + progressBarHeight + 4, timeBuf);
    }
  }

  // Draw page indicators if there are multiple pages
  const int totalPages = (itemCount + itemsPerPage - 1) / itemsPerPage;
  if (totalPages > 1) {
    constexpr int indicatorWidth = 20;
    constexpr int arrowSize = 6;
    constexpr int margin = 15;

    const int centerX = rect.x + rect.width - indicatorWidth / 2 - margin;
    const int indicatorTop = rect.y;
    const int indicatorBottom = rect.y + rect.height - arrowSize;

    // Up arrow
    for (int j = 0; j < arrowSize; ++j) {
      const int lineWidth = 1 + j * 2;
      const int startX = centerX - j;
      renderer.drawLine(startX, indicatorTop + j, startX + lineWidth - 1, indicatorTop + j);
    }

    // Down arrow
    for (int j = 0; j < arrowSize; ++j) {
      const int lineWidth = 1 + (arrowSize - 1 - j) * 2;
      const int startX = centerX - (arrowSize - 1 - j);
      renderer.drawLine(startX, indicatorBottom - arrowSize + 1 + j, startX + lineWidth - 1,
                        indicatorBottom - arrowSize + 1 + j);
    }
  }
}

int GridHomeTheme::getRecentBooksItemsPerPage(const GfxRenderer& renderer) const {
  constexpr int columns = 3;
  constexpr int cellPadding = 8;
  constexpr int progressBarHeight = 4;
  constexpr int titleAreaHeight = 36;
  constexpr int progressAreaHeight = progressBarHeight + 20;

  const int coverHeight = GridHomeMetrics::values.homeCoverHeight;
  const int cellHeight = coverHeight + titleAreaHeight + progressAreaHeight + cellPadding * 2;

  const auto& metrics = UITheme::getInstance().getMetrics();
  const int reservedHeight = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing +
                             metrics.verticalSpacing + metrics.buttonHintsHeight;
  const int availableHeight = renderer.getScreenHeight() - reservedHeight;
  const int rowsPerPage = availableHeight / cellHeight;
  return rowsPerPage * columns;
}
