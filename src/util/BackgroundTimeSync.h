#pragma once

// Quietly re-syncs the clock (and weather) over the last used WiFi network after a deep sleep that lost the RTC.
//
// Keeping the RTC powered through deep sleep ("Keep clock in sleep") keeps the whole board on and costs far more
// battery than an occasional few-second WiFi sync, so this is the low-power way to keep the clock right.
// Runs in a background task; WiFi-using screens call stop() before touching WiFi themselves.
namespace BackgroundTimeSync {

// Starts a sync if enabled in settings, the clock is only an estimate, a saved network is known and the last
// automatic sync is old enough. Safe to call repeatedly; returns immediately.
void maybeStart();

// Cancels a running sync and waits (briefly) until WiFi has been released.
void stop();

bool isRunning();

}  // namespace BackgroundTimeSync
