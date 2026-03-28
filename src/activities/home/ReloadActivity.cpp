#include "ReloadActivity.h"
#include <GfxRenderer.h>
#include <WiFi.h>
#include "../network/WifiSelectionActivity.h"

void ReloadActivity::onEnter() {
  ActivityWithSubactivity::onEnter();

  // Launch WiFi selection immediately
  // autoConnect = true, syncOnConnect = true
  enterNewActivity(new WifiSelectionActivity(renderer, mappedInput, [this](bool connected) {
    onWifiSelectionComplete(connected);
  }, true, true));
}

void ReloadActivity::onWifiSelectionComplete(bool connected) {
  // After WiFi selection is done (connected or cancelled), go back
  exitActivity();
  onGoBack();
}

void ReloadActivity::loop() {
  if (subActivity) {
    subActivity->loop();
  }
}

void ReloadActivity::render(Activity::RenderLock&& lock) {
  // Subactivity handles its own rendering
  if (subActivity) {
    subActivity->render(std::move(lock));
  }
}
