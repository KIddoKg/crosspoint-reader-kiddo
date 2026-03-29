#include "GridHomeTheme.h"

#include <GfxRenderer.h>
#include <HalStorage.h>
#include <ctime>

#include "RecentBooksStore.h"
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
