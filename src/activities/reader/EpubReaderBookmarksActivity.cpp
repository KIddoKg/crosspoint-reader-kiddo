#include "EpubReaderBookmarksActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>
#include <cstdio>
#include <string>

#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr int lineHeight = 30;
constexpr int listTop = 60;
constexpr unsigned long deleteHoldMs = 800;
}  // namespace

int EpubReaderBookmarksActivity::getPageItems() const {
  const int available = renderer.getScreenHeight() - listTop - lineHeight - 40;
  return std::max(1, available / lineHeight);
}

void EpubReaderBookmarksActivity::onEnter() {
  Activity::onEnter();
  selectorIndex = 0;
  requestUpdate();
}

void EpubReaderBookmarksActivity::loop() {
  const int total = static_cast<int>(bookmarks.all().size());

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    onGoBack();
    return;
  }

  if (total > 0 && !deleteHandled && mappedInput.isPressed(MappedInputManager::Button::Confirm) &&
      mappedInput.getHeldTime() >= deleteHoldMs) {
    deleteHandled = true;
    {
      RenderLock lock(*this);
      bookmarks.remove(static_cast<size_t>(selectorIndex));
      selectorIndex = std::min(selectorIndex, static_cast<int>(bookmarks.all().size()) - 1);
      selectorIndex = std::max(selectorIndex, 0);
    }
    requestUpdate();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (deleteHandled) {
      deleteHandled = false;
    } else if (total > 0) {
      onSelect(bookmarks.all()[selectorIndex]);
    }
    return;
  }

  if (total == 0) return;
  const int pageItems = getPageItems();
  buttonNavigator.onNextRelease([this, total] {
    selectorIndex = ButtonNavigator::nextIndex(selectorIndex, total);
    requestUpdate();
  });
  buttonNavigator.onPreviousRelease([this, total] {
    selectorIndex = ButtonNavigator::previousIndex(selectorIndex, total);
    requestUpdate();
  });
  buttonNavigator.onNextContinuous([this, total, pageItems] {
    selectorIndex = ButtonNavigator::nextPageIndex(selectorIndex, total, pageItems);
    requestUpdate();
  });
  buttonNavigator.onPreviousContinuous([this, total, pageItems] {
    selectorIndex = ButtonNavigator::previousPageIndex(selectorIndex, total, pageItems);
    requestUpdate();
  });
}

void EpubReaderBookmarksActivity::render(Activity::RenderLock&&) {
  renderer.clearScreen();
  const int pageWidth = renderer.getScreenWidth();
  renderer.drawCenteredText(UI_12_FONT_ID, 15, tr(STR_BOOKMARKS), true, EpdFontFamily::BOLD);

  const auto& items = bookmarks.all();
  if (items.empty()) {
    renderer.drawCenteredText(UI_10_FONT_ID, listTop + 40, tr(STR_NO_BOOKMARKS), true);
  } else {
    const int pageItems = getPageItems();
    const int pageStart = selectorIndex / pageItems * pageItems;
    renderer.fillRect(0, listTop + (selectorIndex % pageItems) * lineHeight - 2, pageWidth - 1, lineHeight);

    for (int i = 0; i < pageItems && pageStart + i < static_cast<int>(items.size()); i++) {
      const auto& b = items[pageStart + i];
      const bool selected = pageStart + i == selectorIndex;
      const int y = listTop + i * lineHeight;

      char percent[8];
      snprintf(percent, sizeof(percent), "%u%%", static_cast<unsigned>(b.percent));
      const int percentWidth = renderer.getTextWidth(UI_10_FONT_ID, percent);
      renderer.drawText(UI_10_FONT_ID, pageWidth - 20 - percentWidth, y, percent, !selected);

      const int tocIndex = epub->getTocIndexForSpineIndex(b.spineIndex);
      std::string title = tocIndex >= 0 ? epub->getTocItem(tocIndex).title : std::string();
      if (title.empty()) title = std::to_string(b.spineIndex + 1);
      char label[160];
      snprintf(label, sizeof(label), "%s - p.%u", title.c_str(), static_cast<unsigned>(b.page + 1));
      const auto fitted = renderer.truncatedText(UI_10_FONT_ID, label, pageWidth - 60 - percentWidth);
      renderer.drawText(UI_10_FONT_ID, 20, y, fitted.c_str(), !selected);
    }
  }

  if (!items.empty()) {
    // The button hint only fits one short word, so the delete gesture is explained on its own line above it
    const int helpY = renderer.getScreenHeight() - UITheme::getInstance().getMetrics().buttonHintsHeight -
                      renderer.getLineHeight(SMALL_FONT_ID) - 8;
    renderer.drawCenteredText(SMALL_FONT_ID, helpY, tr(STR_BOOKMARK_DELETE_HELP), true);
  }
  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}
