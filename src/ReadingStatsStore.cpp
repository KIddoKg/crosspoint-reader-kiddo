#include "ReadingStatsStore.h"
#include <Arduino.h>
#include <time.h>
#include <algorithm>
#include "CrossPointSettings.h"
#include "activities/reader/FurthestProgress.h"
#include "util/TimeManager.h"
#include "JsonSettingsIO.h"
#include "HalStorage.h"
#include "Logging.h"

#define READING_STATS_FILE "/.crosspoint/reading_stats.json"

ReadingStatsStore::ReadingStatsStore() {
    loadFromFile();
}

ReadingStatsStore& ReadingStatsStore::getInstance() {
    static ReadingStatsStore instance;
    return instance;
}

uint32_t ReadingStatsStore::getTodayIndex() const {
    time_t now = TimeManager::getInstance().getCurrentTime();
    struct tm* t = localtime(&now);
    t->tm_hour = 0;
    t->tm_min = 0;
    t->tm_sec = 0;
    t->tm_isdst = -1;
    return (uint32_t)(mktime(t) / 86400);
}

void ReadingStatsStore::addReadingTime(const std::string& path, const std::string& title, uint32_t minutes, uint8_t progress) {
    if (minutes == 0 || !SETTINGS.readingStatsEnabled) return;

    totalMinutes += minutes;

    // Without a valid clock the day index would be bogus (1970), so only per-day stats are skipped
    if (TimeManager::getInstance().hasValidClock()) {
        const uint32_t today = getTodayIndex();
        bool dailyFound = false;
        for (auto& ds : dailyReadings) {
            if (ds.dayIndex == today) {
                ds.minutes += minutes;
                dailyFound = true;
                break;
            }
        }
        if (!dailyFound) {
            dailyReadings.push_back({today, minutes});
            // Keep the last 30 days of history: drop the oldest day (the vector is not kept in order)
            if (dailyReadings.size() > 30) {
                dailyReadings.erase(std::min_element(
                    dailyReadings.begin(), dailyReadings.end(),
                    [](const DailyStat& a, const DailyStat& b) { return a.dayIndex < b.dayIndex; }));
            }
        }
    }

    bool bookFound = false;
    for (auto& bs : bookReadings) {
        if (bs.path == path) {
            bs.totalMinutes += minutes;
            bs.progress = progress;
            bookFound = true;
            break;
        }
    }
    if (!bookFound) {
        bookReadings.push_back({path, title, minutes, progress});
        // Keep top N books? Or just all for now.
    }

    calculateStreak();
    saveToFile();
}

void ReadingStatsStore::calculateStreak() {
    if (dailyReadings.empty()) {
        currentStreak = 0;
        return;
    }

    // Sort by dayIndex desc
    std::sort(dailyReadings.begin(), dailyReadings.end(), [](const DailyStat& a, const DailyStat& b) {
        return a.dayIndex > b.dayIndex;
    });

    uint32_t today = getTodayIndex();
    uint32_t streak = 0;
    uint32_t expectedDay = today;

    // Check if we read today or yesterday to continue streak
    if (dailyReadings[0].dayIndex < today - 1) {
        currentStreak = 0;
        return;
    }

    if (dailyReadings[0].dayIndex == today - 1) {
        expectedDay = today - 1;
    }

    for (const auto& ds : dailyReadings) {
        if (ds.dayIndex == expectedDay) {
            streak++;
            expectedDay--;
        } else if (ds.dayIndex < expectedDay) {
            break;
        }
    }

    currentStreak = streak;
}

uint32_t ReadingStatsStore::getTodayMinutes() const {
    uint32_t today = getTodayIndex();
    for (const auto& ds : dailyReadings) {
        if (ds.dayIndex == today) return ds.minutes;
    }
    return 0;
}

uint32_t ReadingStatsStore::getBookMinutes(const std::string& path) const {
    for (const auto& bs : bookReadings) {
        if (bs.path == path) {
            return bs.totalMinutes;
        }
    }
    return 0;
}

uint32_t ReadingStatsStore::getFinishedCount() const {
    return static_cast<uint32_t>(std::count_if(bookReadings.begin(), bookReadings.end(), [](const BookStat& b) {
        return b.progress >= FurthestProgress::FINISHED_PERCENT;
    }));
}

std::vector<BookStat> ReadingStatsStore::getTopBooks(size_t count) const {
    std::vector<BookStat> sorted = bookReadings;
    std::sort(sorted.begin(), sorted.end(), [](const BookStat& a, const BookStat& b) {
        return a.totalMinutes > b.totalMinutes;
    });
    if (sorted.size() > count) {
        sorted.resize(count);
    }
    return sorted;
}

void ReadingStatsStore::notePageTurn() {
    const unsigned long now = millis();
    const unsigned long previous = lastPageTurnMs;
    lastPageTurnMs = now;
    if (previous == 0) return;
    // Ignore skimming (< 3 s) and breaks (> 10 min) so the pace reflects actual reading
    const uint32_t seconds = (now - previous) / 1000;
    if (seconds < 3 || seconds > 600) return;
    avgSecondsPerPage = avgSecondsPerPage == 0 ? seconds : (avgSecondsPerPage * 7 + seconds) / 8;
}

int ReadingStatsStore::estimateMinutes(const uint32_t pages) const {
    if (avgSecondsPerPage == 0) return -1;
    return static_cast<int>((pages * avgSecondsPerPage + 59) / 60);
}

void ReadingStatsStore::reset() {
    totalMinutes = 0;
    dailyReadings.clear();
    bookReadings.clear();
    currentStreak = 0;
    lastReadDayIndex = 0;
    avgSecondsPerPage = 0;
    saveToFile();
}

bool ReadingStatsStore::saveToFile() const {
    Storage.mkdir("/.crosspoint");
    return JsonSettingsIO::saveReadingStats(*this, READING_STATS_FILE);
}

bool ReadingStatsStore::loadFromFile() {
    if (!Storage.exists(READING_STATS_FILE)) return false;
    return JsonSettingsIO::loadReadingStats(*this, READING_STATS_FILE);
}
