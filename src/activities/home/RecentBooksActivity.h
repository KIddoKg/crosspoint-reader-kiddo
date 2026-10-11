#pragma once
#include <I18n.h>

#include <functional>
#include <string>
#include <vector>

#include "../Activity.h"
#include "RecentBooksStore.h"
#include "util/ButtonNavigator.h"

// "Bookshelf": books grouped into Reading / Finished (from the recent books list and the furthest point read)
// and Unread (books on the SD card that were never opened).
class RecentBooksActivity final : public Activity {
 private:
  enum Tab { TAB_READING = 0, TAB_FINISHED, TAB_UNREAD, TAB_COUNT };

  ButtonNavigator buttonNavigator;

  int currentTab = TAB_READING;
  bool tabBarFocused = false;  // selection is on the tab bar (Confirm switches tab) instead of a book
  size_t selectorIndex = 0;

  // Books of the current tab
  std::vector<RecentBook> books;

  // Callbacks
  const std::function<void(const std::string& path)> onSelectBook;
  const std::function<void()> onGoHome;

  void loadTab();

 public:
  explicit RecentBooksActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                               const std::function<void()>& onGoHome,
                               const std::function<void(const std::string& path)>& onSelectBook)
      : Activity("RecentBooks", renderer, mappedInput), onSelectBook(onSelectBook), onGoHome(onGoHome) {}
  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(Activity::RenderLock&&) override;
};
