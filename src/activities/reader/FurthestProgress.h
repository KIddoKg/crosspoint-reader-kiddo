#pragma once

#include <HalStorage.h>

#include <cstdint>
#include <string>

#include "ProgressFile.h"

// Tracks the furthest point (book percent) a reader has reached in a book, stored next to progress.bin as
// furthest.bin. Unlike the current position it never goes back when the reader jumps to an earlier chapter, so
// it tells whether a book has been read to the end.
//
// Only sequential reading raises it: the reader calls markSequentialAdvance() on a forward page turn (button or
// auto turn) or a progress sync, then onPageShown() once the new page is rendered. Jumps via the table of
// contents or "go to percent" do not count, so peeking at the last chapter does not mark a book as finished.
class FurthestProgress {
  static constexpr const char* FILE_NAME = "furthest.bin";
  static constexpr uint8_t FILE_VERSION = 1;

  std::string cachePath;
  int percent = -1;  // -1 = no record yet; initialised from the first page shown
  bool pendingAdvance = false;

  void save() const {
    const uint8_t data[2] = {FILE_VERSION, static_cast<uint8_t>(percent)};
    ProgressFile::writeAtomicFile(cachePath, FILE_NAME, data, sizeof(data));
  }

 public:
  static constexpr int FINISHED_PERCENT = 98;

  void load(const std::string& bookCachePath) {
    cachePath = bookCachePath;
    percent = -1;
    pendingAdvance = false;
    HalFile f;
    if (Storage.openFileForRead("FUR", cachePath + "/" + FILE_NAME, f)) {
      uint8_t data[2] = {0, 0};
      if (f.read(data, sizeof(data)) == sizeof(data) && data[0] == FILE_VERSION && data[1] <= 100) {
        percent = data[1];
      }
    }
  }

  // After the book cache was wiped (furthest.bin deleted), fall back to the copy kept in the recent books list
  void seedIfMissing(const uint8_t knownPercent) {
    if (percent < 0 && knownPercent > 0) {
      percent = knownPercent > 100 ? 100 : knownPercent;
      save();
    }
  }

  // Rewrite the file, e.g. after the book's cache directory was cleared
  void persist() const {
    if (percent >= 0) save();
  }

  void markSequentialAdvance() { pendingAdvance = true; }

  // Call after a page is shown, with the book percent at the end of that page (100 on the last page)
  void onPageShown(int currentPercent) {
    if (currentPercent < 0) currentPercent = 0;
    if (currentPercent > 100) currentPercent = 100;
    const bool advance = pendingAdvance;
    pendingAdvance = false;
    if (percent < 0 || (advance && currentPercent > percent)) {
      percent = currentPercent;
      save();
    }
  }

  // "Read again from the start": forget the furthest point and restart from the given position
  void reset(const int currentPercent) {
    percent = currentPercent < 0 ? 0 : currentPercent;
    pendingAdvance = false;
    save();
  }

  // Furthest percent, or `fallback` when nothing has been recorded yet
  uint8_t get(const uint8_t fallback) const { return percent < 0 ? fallback : static_cast<uint8_t>(percent); }
};
