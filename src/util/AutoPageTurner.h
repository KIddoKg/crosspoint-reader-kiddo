#pragma once

#include <Arduino.h>

#include "CrossPointSettings.h"

// Shared timer for the "auto page turn" reader feature (EPUB, TXT and XTC readers).
//
// Call onUserInput() whenever the reader sees a button press, and shouldTurnPage() once per loop while the
// reader page is in the foreground (not while a menu or other sub-activity is open). Auto turning pauses after
// SETTINGS.autoPageTurnIdleStop of no button presses so the book does not run to its end while nobody reads.
class AutoPageTurner {
  unsigned long elapsedMs = 0;
  unsigned long lastTickMs = 0;
  unsigned long lastInputMs = 0;

 public:
  void onUserInput() {
    lastInputMs = millis();
    elapsedMs = 0;
    lastTickMs = lastInputMs;
  }

  bool shouldTurnPage() {
    const unsigned long now = millis();
    if (lastInputMs == 0) lastInputMs = now;
    if (lastTickMs == 0) lastTickMs = now;

    const unsigned long idleStopMs = SETTINGS.getAutoPageTurnIdleStopMs();
    if (!SETTINGS.autoPageTurnEnabled || (idleStopMs > 0 && now - lastInputMs >= idleStopMs)) {
      elapsedMs = 0;
      lastTickMs = now;
      return false;
    }

    elapsedMs += now - lastTickMs;
    lastTickMs = now;
    if (elapsedMs < static_cast<unsigned long>(SETTINGS.autoPageTurnDelay) * 1000UL) {
      return false;
    }
    elapsedMs = 0;
    return true;
  }

  // Restart the delay without counting it as user activity (e.g. after returning from a menu)
  void restartDelay() {
    elapsedMs = 0;
    lastTickMs = millis();
  }
};
