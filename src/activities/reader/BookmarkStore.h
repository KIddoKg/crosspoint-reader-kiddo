#pragma once

#include <HalStorage.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "ProgressFile.h"

// Per-book EPUB bookmarks, stored as bookmarks.bin in the book's cache directory (written atomically).
// A bookmark keeps the page together with the chapter's page count at the time, so it can be mapped to the same
// relative position after a font or layout change re-paginates the chapter.
class BookmarkStore {
 public:
  struct Bookmark {
    uint16_t spineIndex;
    uint16_t page;
    uint16_t pageCount;
    uint8_t percent;  // book percent, for display
  };

  static constexpr size_t MAX_BOOKMARKS = 100;

 private:
  static constexpr const char* FILE_NAME = "bookmarks.bin";
  static constexpr uint8_t FILE_VERSION = 1;
  static constexpr size_t ENTRY_SIZE = 7;

  std::string cachePath;
  std::vector<Bookmark> bookmarks;

  void save() const {
    std::vector<uint8_t> data;
    data.reserve(2 + bookmarks.size() * ENTRY_SIZE);
    data.push_back(FILE_VERSION);
    data.push_back(static_cast<uint8_t>(bookmarks.size()));
    for (const auto& b : bookmarks) {
      data.push_back(b.spineIndex & 0xFF);
      data.push_back(b.spineIndex >> 8);
      data.push_back(b.page & 0xFF);
      data.push_back(b.page >> 8);
      data.push_back(b.pageCount & 0xFF);
      data.push_back(b.pageCount >> 8);
      data.push_back(b.percent);
    }
    ProgressFile::writeAtomicFile(cachePath, FILE_NAME, data.data(), data.size());
  }

  // Page `b` points to once its chapter has `pageCount` pages
  static int mappedPage(const Bookmark& b, const int pageCount) {
    if (b.pageCount == 0 || pageCount <= 0 || b.pageCount == pageCount) return b.page;
    return std::min(pageCount - 1, static_cast<int>(static_cast<long>(b.page) * pageCount / b.pageCount));
  }

 public:
  void load(const std::string& bookCachePath) {
    cachePath = bookCachePath;
    bookmarks.clear();
    HalFile f;
    if (!Storage.openFileForRead("BMK", cachePath + "/" + FILE_NAME, f)) return;
    uint8_t header[2];
    if (f.read(header, sizeof(header)) != sizeof(header) || header[0] != FILE_VERSION) return;
    for (int i = 0; i < header[1]; i++) {
      uint8_t e[ENTRY_SIZE];
      if (f.read(e, sizeof(e)) != sizeof(e)) break;
      bookmarks.push_back({static_cast<uint16_t>(e[0] | (e[1] << 8)), static_cast<uint16_t>(e[2] | (e[3] << 8)),
                           static_cast<uint16_t>(e[4] | (e[5] << 8)), e[6]});
    }
  }

  const std::vector<Bookmark>& all() const { return bookmarks; }

  // Rewrite the file, e.g. after the book's cache directory was cleared
  void persist() const {
    if (!bookmarks.empty()) save();
  }

  bool contains(const int spineIndex, const int page, const int pageCount) const {
    return std::any_of(bookmarks.begin(), bookmarks.end(), [&](const Bookmark& b) {
      return b.spineIndex == spineIndex && mappedPage(b, pageCount) == page;
    });
  }

  // Adds a bookmark for the page, or removes it when the page is already bookmarked. Returns true when added.
  bool toggle(const int spineIndex, const int page, const int pageCount, const uint8_t percent) {
    const auto it = std::find_if(bookmarks.begin(), bookmarks.end(), [&](const Bookmark& b) {
      return b.spineIndex == spineIndex && mappedPage(b, pageCount) == page;
    });
    bool added = false;
    if (it != bookmarks.end()) {
      bookmarks.erase(it);
    } else {
      if (bookmarks.size() >= MAX_BOOKMARKS) bookmarks.erase(bookmarks.begin());
      bookmarks.push_back({static_cast<uint16_t>(spineIndex), static_cast<uint16_t>(page),
                           static_cast<uint16_t>(pageCount), percent});
      std::sort(bookmarks.begin(), bookmarks.end(), [](const Bookmark& a, const Bookmark& b) {
        return a.spineIndex != b.spineIndex ? a.spineIndex < b.spineIndex
                                            : a.page * 1.0f / std::max<int>(a.pageCount, 1) <
                                                  b.page * 1.0f / std::max<int>(b.pageCount, 1);
      });
      added = true;
    }
    save();
    return added;
  }

  void remove(const size_t index) {
    if (index >= bookmarks.size()) return;
    bookmarks.erase(bookmarks.begin() + static_cast<long>(index));
    save();
  }

  // Position of a bookmark within its chapter (0..1), applied once the chapter is laid out
  static float chapterFraction(const Bookmark& b) {
    return b.pageCount == 0 ? 0.0f : static_cast<float>(b.page) / static_cast<float>(b.pageCount);
  }
};
