#pragma once

#include "components/themes/lyra/LyraTheme.h"

class GfxRenderer;

// Grid Home theme metrics - based on Lyra with optimizations for grid layout
namespace GridHomeMetrics {
constexpr ThemeMetrics values = {.batteryWidth = 16,
                                  .batteryHeight = 12,
                                  .topPadding = 5,
                                  .batteryBarHeight = 40,
                                  .headerHeight = 84,
                                  .verticalSpacing = 12,  // Giảm khoảng cách lại một chút theo ý bạn
                                  .contentSidePadding = 20,
                                  .listRowHeight = 40,
                                  .listWithSubtitleRowHeight = 60,
                                  .menuRowHeight = 110,  // Grid tile height
                                  .menuSpacing = 12,    // Spacing between tiles
                                  .tabSpacing = 8,
                                  .tabBarHeight = 40,
                                  .scrollBarWidth = 4,
                                  .scrollBarRightOffset = 5,
                                  .homeTopPadding = 56,
                                  .homeCoverHeight = 180,      // Hiển thị tối đa 3 cuốn
                                  .homeCoverTileHeight = 225,  // Chiều cao tổng của khu vực 1 cuốn truyện
                                  .homeRecentBooksCount = 3,   // Hiển thị tối đa 3 truyện gần nhất
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

// Grid Home theme - extends Lyra with grid-based menu layout
class GridHomeTheme : public LyraTheme {
 public:
  // Override to support up to 3 books horizontally
  void drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                           int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                           std::function<bool()> storeCoverBuffer) const override;

  // Override grid menu drawing to use 2-column layout
  void drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                      const std::function<std::string(int index)>& buttonLabel,
                      const std::function<UIIcon(int index)>& rowIcon) const override;
};
