#pragma once
#include <Epub.h>

#include <functional>
#include <memory>

#include "../Activity.h"
#include "BookmarkStore.h"
#include "util/ButtonNavigator.h"

// Lists a book's bookmarks (chapter title + percent). Confirm jumps to one, holding Confirm deletes it.
class EpubReaderBookmarksActivity final : public Activity {
  std::shared_ptr<Epub> epub;
  BookmarkStore& bookmarks;
  ButtonNavigator buttonNavigator;
  int selectorIndex = 0;
  bool deleteHandled = false;  // a hold already deleted the item; ignore the matching release

  const std::function<void()> onGoBack;
  const std::function<void(const BookmarkStore::Bookmark&)> onSelect;

  int getPageItems() const;

 public:
  explicit EpubReaderBookmarksActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                       const std::shared_ptr<Epub>& epub, BookmarkStore& bookmarks,
                                       const std::function<void()>& onGoBack,
                                       const std::function<void(const BookmarkStore::Bookmark&)>& onSelect)
      : Activity("EpubReaderBookmarks", renderer, mappedInput),
        epub(epub),
        bookmarks(bookmarks),
        onGoBack(onGoBack),
        onSelect(onSelect) {}
  void onEnter() override;
  void loop() override;
  void render(Activity::RenderLock&&) override;
};
