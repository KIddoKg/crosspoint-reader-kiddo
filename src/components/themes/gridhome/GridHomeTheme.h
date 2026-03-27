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
                                  .verticalSpacing = 16,
                                  .contentSidePadding = 20,
                                  .listRowHeight = 40,
                                  .listWithSubtitleRowHeight = 60,
                                  .menuRowHeight = 110,  // Grid tile height - larger for icon + text layout
                                  .menuSpacing = 12,    // Spacing between tiles
                                  .tabSpacing = 8,
                                  .tabBarHeight = 40,
                                  .scrollBarWidth = 4,
                                  .scrollBarRightOffset = 5,
                                  .homeTopPadding = 56,
                                  .homeCoverHeight = 226,
                                  .homeCoverTileHeight = 242,
                                  .homeRecentBooksCount = 2,  // Show 2 recent books
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
  // Override grid menu drawing to use 2-column layout
  void drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                      const std::function<std::string(int index)>& buttonLabel,
                      const std::function<UIIcon(int index)>& rowIcon) const override;
};
