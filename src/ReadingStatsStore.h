#pragma once

#include <string>
#include <vector>
#include <map>

struct BookStat {
    std::string path;
    std::string title;
    uint32_t totalMinutes = 0;
    uint8_t progress = 0;
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
    uint32_t getBookMinutes(const std::string& path) const;
    std::vector<BookStat> getTopBooks(size_t count) const;

    bool loadFromFile();
    bool saveToFile() const;
};

#define READING_STATS ReadingStatsStore::getInstance()
