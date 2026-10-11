#pragma once

#include <string>
#include <vector>
#include <map>

struct BookStat {
    std::string path;
    std::string title;
    uint32_t totalMinutes = 0;
    uint8_t progress = 0;  // furthest point read (percent)
};

struct DailyStat {
    uint32_t dayIndex = 0;
    uint32_t minutes = 0;
};

class ReadingStatsStore;
namespace JsonSettingsIO {
bool saveReadingStats(const ReadingStatsStore& store, const char* path);
bool loadReadingStats(ReadingStatsStore& store, const char* path);
}  // namespace JsonSettingsIO

class ReadingStatsStore {
private:
    uint32_t totalMinutes = 0;
    std::vector<DailyStat> dailyReadings; 
    std::vector<BookStat> bookReadings;
    uint32_t currentStreak = 0;
    uint32_t lastReadDayIndex = 0;
    // Reading pace: moving average of seconds spent per page (manual page turns only), 0 = unknown
    uint32_t avgSecondsPerPage = 0;
    unsigned long lastPageTurnMs = 0;

    ReadingStatsStore();
    void calculateStreak();
    uint32_t getTodayIndex() const;

    friend bool JsonSettingsIO::saveReadingStats(const ReadingStatsStore&, const char*);
    friend bool JsonSettingsIO::loadReadingStats(ReadingStatsStore&, const char*);

public:
    static ReadingStatsStore& getInstance();

    void addReadingTime(const std::string& path, const std::string& title, uint32_t minutes, uint8_t progress);
    
    uint32_t getTodayMinutes() const;
    uint32_t getTotalMinutes() const { return totalMinutes; }
    uint32_t getStreak() const { return currentStreak; }
    uint32_t getAvgSecondsPerPage() const { return avgSecondsPerPage; }
    // Call on every manual forward page turn; feeds the reading pace used for "time left" estimates.
    // Kept in memory and persisted with the next stats save.
    void notePageTurn();
    // Minutes needed for `pages` pages at the reader's pace, or -1 when the pace is unknown
    int estimateMinutes(uint32_t pages) const;
    uint32_t getBookMinutes(const std::string& path) const;
    std::vector<BookStat> getTopBooks(size_t count) const;
    // Books whose furthest read point reached the end (see FurthestProgress::FINISHED_PERCENT)
    uint32_t getFinishedCount() const;

    // Clears all statistics and persists the empty state
    void reset();

    bool loadFromFile();
    bool saveToFile() const;
};

#define READING_STATS ReadingStatsStore::getInstance()
