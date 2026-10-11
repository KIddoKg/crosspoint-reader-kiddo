#pragma once

#include "components/themes/BaseTheme.h"

// Minimal theme metrics: Classic with a compact text "now reading" block instead of a large cover
namespace MinimalMetrics {
constexpr ThemeMetrics values = {.batteryWidth = 15,
                                 .batteryHeight = 12,
                                 .topPadding = 5,
                                 .batteryBarHeight = 20,
                                 .headerHeight = 45,
                                 .verticalSpacing = 10,
                                 .contentSidePadding = 24,
                                 .listRowHeight = 30,
                                 .listWithSubtitleRowHeight = 65,
                                 .menuRowHeight = 40,
                                 .menuSpacing = 4,
                                 .tabSpacing = 10,
                                 .tabBarHeight = 50,
                                 .scrollBarWidth = 4,
                                 .scrollBarRightOffset = 5,
                                 .homeTopPadding = 40,
                                 .homeCoverHeight = 180,  // no cover on Home; matches other themes' thumbnails
                                 .homeCoverTileHeight = 170,
                                 .homeRecentBooksCount = 1,
                                 .buttonHintsHeight = 40,
                                 .sideButtonHintsWidth = 30,
                                 .progressBarHeight = 16,
                                 .bookProgressBarHeight = 4,
                                 .keyboardKeyWidth = 22,
                                 .keyboardKeyHeight = 30,
                                 .keyboardKeySpacing = 10,
                                 .keyboardBottomAligned = false,
                                 .keyboardCenteredText = false};
}

// "Minimal": text only, no boxes or covers on Home: fast to draw and leaves little ghosting on e-ink.
// A quote from /quotes.txt (one per line, picked at random each wake) fills the bottom of the Home screen.
class MinimalTheme : public BaseTheme {
 public:
  void drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                           int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                           std::function<bool()> storeCoverBuffer) const override;

  void drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                      const std::function<std::string(int index)>& buttonLabel,
                      const std::function<UIIcon(int index)>& rowIcon) const override;
};
