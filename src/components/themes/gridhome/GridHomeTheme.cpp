#include "GridHomeTheme.h"

#include <GfxRenderer.h>
#include <ctime>

#include "components/icons/file24.h"
#include "components/icons/folder24.h"
#include "components/icons/image24.h"
#include "components/icons/library.h"
#include "components/icons/recent.h"
#include "components/icons/settings2.h"
#include "components/icons/text24.h"
#include "components/icons/transfer.h"
#include "components/icons/wifi.h"
#include "components/icons/reload.h"
#include "components/icons/folder.h"
#include "components/icons/file24.h"
#include "components/icons/book.h"
#include "fontIds.h"
#include "util/TimeManager.h"

namespace {
constexpr int cornerRadius = 16;
constexpr int tilePadding = 16;  // Padding around icon and text inside tile
constexpr int mainMenuIconSize = 32;
constexpr int mainMenuColumns = 2;

const uint8_t* iconForName(UIIcon icon) {
  switch (icon) {
    case UIIcon::Folder:
      return FolderIcon;  // TODO: Add folder icon if needed
    case UIIcon::Text:
      return nullptr;
    case UIIcon::Image:
      return nullptr;
    case UIIcon::Book:
      return BookIcon;
    case UIIcon::File:
      return File24Icon;
    case UIIcon::Recent:
      return RecentIcon;
    case UIIcon::Settings:
      return Settings2Icon;
    case UIIcon::Transfer:
      return TransferIcon;
    case UIIcon::Library:
      return LibraryIcon;

    case UIIcon::Reload:
      return ReloadIcon;
    case UIIcon::Wifi:
      return WifiIcon;
    default:
      return nullptr;
  }
}
}  // namespace

void GridHomeTheme::drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                                   const std::function<std::string(int index)>& buttonLabel,
                                   const std::function<UIIcon(int index)>& rowIcon) const {
  // Draw date/time on the left side - get time from TimeManager
  time_t now = TimeManager::getInstance().getCurrentTime();
  struct tm* timeinfo = localtime(&now);
  char dateTimeBuffer[64];
  strftime(dateTimeBuffer, sizeof(dateTimeBuffer), "%H:%M:%S - %d/%m/%Y", timeinfo);
  
  // Clear the area first to prevent ghosting
  const int textWidth = renderer.getTextWidth(UI_10_FONT_ID, dateTimeBuffer);
  const int textHeight = renderer.getTextHeight(UI_10_FONT_ID);
  renderer.fillRect(rect.x + 5, rect.y + 5, textWidth + 15, textHeight + 8, false);
  
  // Draw text
  renderer.drawText(UI_10_FONT_ID, rect.x + 8, rect.y + 8, dateTimeBuffer, false);
  
  // 2-column grid layout matching new_theme.md
  const int tileWidth = (rect.width - GridHomeMetrics::values.contentSidePadding * 2 - GridHomeMetrics::values.menuSpacing) / mainMenuColumns;
  const int tileHeight = GridHomeMetrics::values.menuRowHeight;

  for (int i = 0; i < buttonCount; ++i) {
    const int col = i % mainMenuColumns;
    const int row = i / mainMenuColumns;

    const int tileX = rect.x + GridHomeMetrics::values.contentSidePadding + col * (tileWidth + GridHomeMetrics::values.menuSpacing);
    const int tileY = rect.y + GridHomeMetrics::values.verticalSpacing + row * (tileHeight + GridHomeMetrics::values.menuSpacing);

    const bool selected = selectedIndex == i;

    // Draw tile with Lyra-style rounded corners
    if (selected) {
      renderer.fillRoundedRect(tileX, tileY, tileWidth, tileHeight, cornerRadius, Color::LightGray);
    } else {
      // Draw black rounded border outline first (2px thickness)
      const int borderThickness = 2;
      renderer.fillRoundedRect(tileX - borderThickness, tileY - borderThickness, tileWidth + borderThickness * 2, tileHeight + borderThickness * 2, cornerRadius + borderThickness, Color::Black);
      // Draw white rounded background on top
      renderer.fillRoundedRect(tileX, tileY, tileWidth, tileHeight, cornerRadius, Color::White);
    }

    // Draw icon centered at top with padding
    if (rowIcon != nullptr) {
      UIIcon icon = rowIcon(i);
      const uint8_t* iconBitmap = iconForName(icon);
      if (iconBitmap != nullptr) {
        const int iconX = tileX + (tileWidth - mainMenuIconSize) / 2;
        const int iconY = tileY + tilePadding;
        renderer.drawIcon(iconBitmap, iconX, iconY, mainMenuIconSize, mainMenuIconSize);
      }
    }

    // Draw text centered at bottom with padding
    std::string labelStr = buttonLabel(i);
    const char* label = labelStr.c_str();
    
    const int textWidth = renderer.getTextWidth(UI_12_FONT_ID, label);
    const int lineHeight = renderer.getLineHeight(UI_12_FONT_ID);
    const int textX = tileX + (tileWidth - textWidth) / 2;
    const int textY = tileY + tileHeight - lineHeight - tilePadding;

    renderer.drawText(UI_12_FONT_ID, textX, textY, label, true);
  }
}
