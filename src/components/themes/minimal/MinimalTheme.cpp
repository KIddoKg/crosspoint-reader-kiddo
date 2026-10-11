#include "MinimalTheme.h"

#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>
#include <esp_random.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "RecentBooksStore.h"
#include "activities/reader/FurthestProgress.h"
#include "components/DashboardWidgets.h"
#include "fontIds.h"

namespace {
constexpr const char* QUOTES_FILE = "/quotes.txt";
constexpr size_t MAX_QUOTES_FILE = 16 * 1024;

const char* const BUILTIN_QUOTES[] = {
    "Đọc sách hay cũng như trò chuyện với người xưa.",
    "Một cuốn sách hay là một người bạn tốt.",
    "Mỗi trang sách là một bước chân đi xa.",
};

// One random quote per boot: from /quotes.txt when present (one per line), otherwise a built-in one
const std::string& quoteOfTheWake() {
  static std::string quote;
  static bool picked = false;
  if (picked) return quote;
  picked = true;

  std::vector<std::string> lines;
  if (Storage.exists(QUOTES_FILE)) {
    const String content = Storage.readFile(QUOTES_FILE);
    std::string text(content.c_str(), std::min<size_t>(content.length(), MAX_QUOTES_FILE));
    size_t start = 0;
    while (start < text.size()) {
      size_t end = text.find('\n', start);
      if (end == std::string::npos) end = text.size();
      std::string line = text.substr(start, end - start);
      while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
      if (!line.empty()) lines.push_back(line);
      start = end + 1;
    }
  }
  if (!lines.empty()) {
    quote = lines[esp_random() % lines.size()];
  } else {
    quote = BUILTIN_QUOTES[esp_random() % (sizeof(BUILTIN_QUOTES) / sizeof(BUILTIN_QUOTES[0]))];
  }
  return quote;
}

// Draws `text` word-wrapped into at most `maxLines` lines; returns the y after the last line
int drawWrapped(const GfxRenderer& renderer, const int fontId, const std::string& text, const int x, int y,
                const int maxWidth, const int maxLines, const EpdFontFamily::Style style) {
  const int lineHeight = renderer.getLineHeight(fontId);
  size_t pos = 0;
  for (int line = 0; line < maxLines && pos < text.size(); line++) {
    std::string current;
    size_t next = pos;
    while (next < text.size()) {
      size_t space = text.find(' ', next);
      if (space == std::string::npos) space = text.size();
      const std::string candidate = current.empty() ? text.substr(next, space - next)
                                                    : current + " " + text.substr(next, space - next);
      if (!current.empty() && renderer.getTextWidth(fontId, candidate.c_str(), style) > maxWidth) break;
      current = candidate;
      next = space + 1;
    }
    if (line == maxLines - 1 && next < text.size()) {
      current = renderer.truncatedText(fontId, (current + " " + text.substr(next)).c_str(), maxWidth, style);
    }
    renderer.drawText(fontId, x, y, current.c_str(), true, style);
    y += lineHeight;
    pos = next;
  }
  return y;
}
}  // namespace

void MinimalTheme::drawRecentBookCover(GfxRenderer& renderer, const Rect rect,
                                       const std::vector<RecentBook>& recentBooks, const int selectorIndex,
                                       bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                                       std::function<bool()> storeCoverBuffer) const {
  const int x = rect.x + MinimalMetrics::values.contentSidePadding;
  const int width = rect.width - 2 * MinimalMetrics::values.contentSidePadding;
  int y = rect.y + 8;

  if (recentBooks.empty()) {
    renderer.drawText(UI_12_FONT_ID, x, y, tr(STR_NO_OPEN_BOOK), true, EpdFontFamily::BOLD);
    renderer.drawText(UI_10_FONT_ID, x, y + renderer.getLineHeight(UI_12_FONT_ID) + 4, tr(STR_START_READING), true);
  } else {
    const RecentBook& book = recentBooks[0];
    const bool selected = selectorIndex == 0;
    if (selected) {
      renderer.fillRect(rect.x + MinimalMetrics::values.contentSidePadding - 8, rect.y, 4, rect.height - 16, true);
    }
    renderer.drawText(UI_10_FONT_ID, x, y, tr(STR_CONTINUE_READING_SHORT), true);
    y += renderer.getLineHeight(UI_10_FONT_ID) + 4;
    y = drawWrapped(renderer, UI_12_FONT_ID, book.title, x, y, width, 2, EpdFontFamily::BOLD) + 4;

    const uint8_t furthest = std::max(book.furthest, book.progress);
    char line[96];
    if (furthest >= FurthestProgress::FINISHED_PERCENT) {
      snprintf(line, sizeof(line), "%s", tr(STR_BOOK_FINISHED));
    } else {
      snprintf(line, sizeof(line), "%u%%", static_cast<unsigned>(furthest));
    }
    const int minutesLeft = DashboardWidgets::chapterMinutesLeft(book);
    if (minutesLeft >= 0) {
      char left[48];
      snprintf(left, sizeof(left), tr(STR_MINUTES_LEFT_CHAPTER), minutesLeft);
      strncat(line, " - ", sizeof(line) - strlen(line) - 1);
      strncat(line, left, sizeof(line) - strlen(line) - 1);
    }
    const auto fitted = renderer.truncatedText(UI_10_FONT_ID, line, width);
    renderer.drawText(UI_10_FONT_ID, x, y, fitted.c_str(), true);
  }

  renderer.drawLine(x, rect.y + rect.height - 6, x + width, rect.y + rect.height - 6, true);
}

void MinimalTheme::drawButtonMenu(GfxRenderer& renderer, const Rect rect, const int buttonCount,
                                  const int selectedIndex, const std::function<std::string(int index)>& buttonLabel,
                                  const std::function<UIIcon(int index)>& rowIcon) const {
  const auto& m = MinimalMetrics::values;
  const int x = rect.x + m.contentSidePadding;
  const int width = rect.width - 2 * m.contentSidePadding;
  const int textOffset = (m.menuRowHeight - renderer.getLineHeight(UI_12_FONT_ID)) / 2;

  int y = rect.y;
  for (int i = 0; i < buttonCount; i++) {
    const bool selected = i == selectedIndex;
    if (selected) {
      renderer.fillRect(x - 8, y, width + 16, m.menuRowHeight, true);
    }
    const auto label = renderer.truncatedText(UI_12_FONT_ID, buttonLabel(i).c_str(), width);
    renderer.drawText(UI_12_FONT_ID, x, y + textOffset, label.c_str(), !selected);
    y += m.menuRowHeight + m.menuSpacing;
  }

  // Quote in the remaining space at the bottom
  const int lineHeight = renderer.getLineHeight(UI_10_FONT_ID);
  // HomeActivity's menu rect can extend past the button hints, so clamp to the visible area
  const int bottom = std::min(rect.y + rect.height,
                              renderer.getScreenHeight() - MinimalMetrics::values.buttonHintsHeight - 10);
  if (bottom - y > lineHeight * 2 + 16) {
    drawWrapped(renderer, UI_10_FONT_ID, "\"" + quoteOfTheWake() + "\"", x, bottom - lineHeight * 3, width, 3,
                EpdFontFamily::REGULAR);
  }
}
