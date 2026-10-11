#pragma once

// TEMPORARY debug logger: appends lines to /.crosspoint/debug.log on the SD card so that
// problems happening while USB is unplugged (deep sleep / wake) can be inspected later.
// Remove once the "jump to last chapter" and "recent books disappear" bugs are fixed.

#define DEBUG_FILE_LOG_PATH "/.crosspoint/debug.log"

void debugFileLog(const char* tag, const char* format, ...) __attribute__((format(printf, 2, 3)));

#define DLOG(tag, format, ...) debugFileLog(tag, format, ##__VA_ARGS__)
