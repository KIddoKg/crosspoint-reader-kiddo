#pragma once

#include "components/themes/gridhome/GridHomeTheme.h"

// Desk theme metrics: GridHome with a single large "current book" card, an info area and a 4-column icon menu
namespace DeskMetrics {
constexpr ThemeMetrics values = {.batteryWidth = 16,
                                 .batteryHeight = 12,
                                 .topPadding = 5,
                                 .batteryBarHeight = 40,
                                 .headerHeight = 84,
                                 .verticalSpacing = 12,
                                 .contentSidePadding = 20,
                                 .listRowHeight = 40,
                                 .listWithSubtitleRowHeight = 60,
                                 .menuRowHeight = 84,  // icon tile height
                                 .menuSpacing = 8,
                                 .tabSpacing = 8,
                                 .tabBarHeight = 40,
                                 .scrollBarWidth = 4,
                                 .scrollBarRightOffset = 5,
                                 .homeTopPadding = 40,
                                 .homeCoverHeight = 180,      // same thumbnails as GridHome (no extra generation)
                                 .homeCoverTileHeight = 450,  // book card + clock + goal blocks
                                 .homeRecentBooksCount = 1,   // only the book being read
                                 .buttonHintsHeight = 40,
                                 .sideButtonHintsWidth = 30,
                                 .progressBarHeight = 16,
                                 .bookProgressBarHeight = 4,
                                 .keyboardKeyWidth = 31,
                                 .keyboardKeyHeight = 50,
                                 .keyboardKeySpacing = 0,
                                 .keyboardBottomAligned = true,
                                 .keyboardCenteredText = true};
}

// "Desk" home: the current book with progress and time left in the chapter, a clock with lunar date and weather,
// today's reading goal, and an icon menu. Lists and the recent books grid come from GridHome.
class DeskTheme : public GridHomeTheme {
 public:
  void drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                           int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                           std::function<bool()> storeCoverBuffer) const override;

  void drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                      const std::function<std::string(int index)>& buttonLabel,
                      const std::function<UIIcon(int index)>& rowIcon) const override;
};
