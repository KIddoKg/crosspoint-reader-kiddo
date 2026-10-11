#include "RecentBooksActivity.h"

#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>

#include <algorithm>
#include <cstring>

#include "MappedInputManager.h"
#include "RecentBooksStore.h"
#include "activities/reader/FurthestProgress.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "util/StringUtils.h"

namespace {
constexpr size_t MAX_UNREAD_BOOKS = 200;
constexpr int MAX_SCAN_DEPTH = 4;

bool isBookFile(const std::string& name) {
  return StringUtils::checkFileExtension(name, ".epub") || StringUtils::checkFileExtension(name, ".txt") ||
         StringUtils::checkFileExtension(name, ".md") || StringUtils::checkFileExtension(name, ".xtc") ||
         StringUtils::checkFileExtension(name, ".xtch");
}

void scanBooks(const std::string& dirPath, const int depth, std::vector<std::string>& out) {
  if (depth > MAX_SCAN_DEPTH || out.size() >= MAX_UNREAD_BOOKS) return;
  auto dir = Storage.open(dirPath.c_str());
  if (!dir || !dir.isDirectory()) return;
  char name[256];
  for (auto file = dir.openNextFile(); file && out.size() < MAX_UNREAD_BOOKS; file = dir.openNextFile()) {
    file.getName(name, sizeof(name));
    const bool isDir = file.isDirectory();
    file.close();
    if (name[0] == '.' || strcmp(name, "System Volume Information") == 0) continue;
    const std::string path = (dirPath == "/" ? "/" : dirPath + "/") + name;
    if (isDir) {
      scanBooks(path, depth + 1, out);
    } else if (isBookFile(name)) {
      out.push_back(path);
    }
  }
}

// SD book paths, scanned once per boot; books added later (e.g. via File Transfer) show up after the next wake
const std::vector<std::string>& libraryBooks() {
  static std::vector<std::string> paths;
  static bool scanned = false;
  if (!scanned) {
    scanBooks("/", 0, paths);
    std::sort(paths.begin(), paths.end());
    scanned = true;
  }
  return paths;
}

uint8_t furthestOf(const RecentBook& book) { return std::max(book.furthest, book.progress); }
}  // namespace

void RecentBooksActivity::loadTab() {
  books.clear();
  if (currentTab == TAB_UNREAD) {
    const auto& recent = RECENT_BOOKS.getBooks();
    for (const auto& path : libraryBooks()) {
      const bool opened =
          std::any_of(recent.begin(), recent.end(), [&](const RecentBook& b) { return b.path == path; });
      if (opened) continue;
      const std::string fileName = path.substr(path.rfind('/') + 1);
      books.push_back({path, fileName.substr(0, fileName.rfind('.')), "", "", 0, 0});
    }
    return;
  }

  const bool wantFinished = currentTab == TAB_FINISHED;
  for (const auto& book : RECENT_BOOKS.getBooks()) {
    if ((furthestOf(book) >= FurthestProgress::FINISHED_PERCENT) != wantFinished) continue;
    if (!Storage.exists(book.path.c_str())) continue;  // Skip if file no longer exists
    books.push_back(book);
  }
}

void RecentBooksActivity::onEnter() {
  Activity::onEnter();
  currentTab = TAB_READING;
  tabBarFocused = false;
  selectorIndex = 0;
  loadTab();
  requestUpdate();
}

void RecentBooksActivity::onExit() {
  Activity::onExit();
  books.clear();
}

void RecentBooksActivity::loop() {
  const int pageItems = GUI.getRecentBooksItemsPerPage(renderer);
  const int listSize = static_cast<int>(books.size());

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    onGoHome();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (tabBarFocused) {
      {
        RenderLock lock(*this);
        currentTab = (currentTab + 1) % TAB_COUNT;
        selectorIndex = 0;
        if (currentTab == TAB_UNREAD) {
          GUI.drawPopup(renderer, tr(STR_LOADING_POPUP));  // first scan of the SD card can take a moment
        }
        loadTab();
      }
      requestUpdate();
    } else if (selectorIndex < books.size()) {
      LOG_DBG("RBA", "Selected book: %s", books[selectorIndex].path.c_str());
      onSelectBook(books[selectorIndex].path);
    }
    return;
  }

  buttonNavigator.onNextRelease([this, listSize] {
    if (tabBarFocused) {
      if (listSize > 0) tabBarFocused = false;
      selectorIndex = 0;
    } else if (listSize > 0) {
      selectorIndex = ButtonNavigator::nextIndex(static_cast<int>(selectorIndex), listSize);
    }
    requestUpdate();
  });

  buttonNavigator.onPreviousRelease([this, listSize] {
    if (!tabBarFocused && (selectorIndex == 0 || listSize == 0)) {
      tabBarFocused = true;
    } else if (!tabBarFocused) {
      selectorIndex = ButtonNavigator::previousIndex(static_cast<int>(selectorIndex), listSize);
    }
    requestUpdate();
  });

  buttonNavigator.onNextContinuous([this, listSize, pageItems] {
    if (tabBarFocused || listSize == 0) return;
    selectorIndex = ButtonNavigator::nextPageIndex(static_cast<int>(selectorIndex), listSize, pageItems);
    requestUpdate();
  });

  buttonNavigator.onPreviousContinuous([this, listSize, pageItems] {
    if (tabBarFocused || listSize == 0) return;
    selectorIndex = ButtonNavigator::previousPageIndex(static_cast<int>(selectorIndex), listSize, pageItems);
    requestUpdate();
  });
}

void RecentBooksActivity::render(Activity::RenderLock&&) {
  renderer.clearScreen();

  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();
  auto metrics = UITheme::getInstance().getMetrics();

  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_BOOKSHELF));

  const int tabY = metrics.topPadding + metrics.headerHeight;
  const std::vector<TabInfo> tabs = {{tr(STR_SHELF_READING), currentTab == TAB_READING},
                                     {tr(STR_BOOK_FINISHED), currentTab == TAB_FINISHED},
                                     {tr(STR_SHELF_UNREAD), currentTab == TAB_UNREAD}};
  GUI.drawTabBar(renderer, Rect{0, tabY, pageWidth, metrics.tabBarHeight}, tabs, tabBarFocused);

  const int contentTop = tabY + metrics.tabBarHeight + metrics.verticalSpacing;
  const int contentHeight = pageHeight - contentTop - metrics.buttonHintsHeight - metrics.verticalSpacing;

  if (books.empty()) {
    renderer.drawText(UI_10_FONT_ID, metrics.contentSidePadding, contentTop + 20, tr(STR_NO_RECENT_BOOKS));
  } else {
    GUI.drawRecentBooksList(renderer, Rect{0, contentTop, pageWidth, contentHeight}, books,
                            tabBarFocused ? -1 : static_cast<int>(selectorIndex));
  }

  // Help text
  const auto labels = mappedInput.mapLabels(tr(STR_HOME), tabBarFocused ? tr(STR_SELECT) : tr(STR_OPEN),
                                            tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
