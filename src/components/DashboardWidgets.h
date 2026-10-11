#pragma once

#include "components/themes/BaseTheme.h"

class GfxRenderer;
struct RecentBook;

// Information blocks shared by the Desk home theme and the "Dashboard" sleep screen
namespace DashboardWidgets {

// Large clock with the Gregorian + lunar date and the last known weather, centered in `rect`.
// With `dateOnly` (sleep screen, where a frozen clock would mislead) the large line shows the date instead.
void drawClock(const GfxRenderer& renderer, Rect rect, bool dateOnly = false);

// Today's reading minutes against the daily goal (bar) and the reading streak
void drawGoal(const GfxRenderer& renderer, Rect rect);

// Estimated minutes to finish the current chapter of an EPUB from its saved position and the reading pace,
// or -1 when unknown (no pace yet, not an EPUB, no saved position)
int chapterMinutesLeft(const RecentBook& book);

}  // namespace DashboardWidgets
