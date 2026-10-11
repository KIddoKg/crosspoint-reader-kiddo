#include "BackgroundTimeSync.h"

#include <Arduino.h>
#include <HalPowerManager.h>
#include <Logging.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <string>

#include "CrossPointSettings.h"
#include "CrossPointState.h"
#include "WifiCredentialStore.h"
#include "util/TimeManager.h"
#include "util/WeatherManager.h"

namespace {
// The estimated clock does not advance during deep sleep, so a long interval here would suppress syncs after long
// sleeps; this only stops repeated syncs within one reading session (quick off/on cycles).
constexpr time_t MIN_SYNC_INTERVAL_S = 30 * 60;
constexpr unsigned long CONNECT_TIMEOUT_MS = 10000;
constexpr unsigned long SYNC_TIMEOUT_MS = 15000;
constexpr uint16_t MIN_BATTERY_PERCENT = 15;

TaskHandle_t taskHandle = nullptr;
volatile bool cancelRequested = false;

// Waits until `done()` or the timeout/cancel; returns true when `done()` became true
template <typename Fn>
bool waitFor(Fn done, const unsigned long timeoutMs) {
  const unsigned long start = millis();
  while (!done()) {
    if (cancelRequested || millis() - start >= timeoutMs) return false;
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  return true;
}

void syncTask(void* param) {
  auto* credential = static_cast<WifiCredential*>(param);
  LOG_INF("BGSYNC", "Connecting to '%s' for time sync", credential->ssid.c_str());

  WiFi.mode(WIFI_STA);
  WiFi.begin(credential->ssid.c_str(), credential->password.c_str());
  delete credential;

  if (waitFor([] { return WiFi.status() == WL_CONNECTED; }, CONNECT_TIMEOUT_MS)) {
    TimeManager::getInstance().setWiFiConnected(true);
    waitFor([] { return !TimeManager::getInstance().isTimeSyncing(); }, SYNC_TIMEOUT_MS);
    if (!TimeManager::getInstance().isApproximate()) {
      APP_STATE.lastAutoSyncTime = static_cast<int64_t>(time(nullptr));
    }
    if (SETTINGS.weatherEnabled && !cancelRequested) {
      WeatherManager::getInstance().setWiFiConnected(true);
      waitFor([] { return !WeatherManager::getInstance().isWeatherSyncing(); }, SYNC_TIMEOUT_MS);
    }
    LOG_INF("BGSYNC", "Sync finished (approximate=%d)", TimeManager::getInstance().isApproximate() ? 1 : 0);
  } else {
    LOG_INF("BGSYNC", "WiFi not reachable, skipping");
  }

  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  TimeManager::getInstance().setWiFiConnected(false);
  taskHandle = nullptr;
  vTaskDelete(nullptr);
}
}  // namespace

namespace BackgroundTimeSync {

bool isRunning() { return taskHandle != nullptr; }

void maybeStart() {
  if (isRunning() || !SETTINGS.autoTimeSync || SETTINGS.timeTrackingInSleep) return;
  if (WiFi.getMode() != WIFI_MODE_NULL) return;  // another screen owns WiFi

  auto& time = TimeManager::getInstance();
  if (time.hasValidClock() && !time.isApproximate()) return;
  const time_t now = ::time(nullptr);
  if (time.hasValidClock() && APP_STATE.lastAutoSyncTime > 0 && now - APP_STATE.lastAutoSyncTime < MIN_SYNC_INTERVAL_S) {
    return;
  }
  if (powerManager.getBatteryPercentage() < MIN_BATTERY_PERCENT) return;

  if (WIFI_STORE.getCredentials().empty()) {
    WIFI_STORE.loadFromFile();
  }
  const auto* credential = WIFI_STORE.findCredential(WIFI_STORE.getLastConnectedSsid());
  if (!credential) return;

  cancelRequested = false;
  // Remember the attempt so an unreachable network is not retried on every wake
  APP_STATE.lastAutoSyncTime = static_cast<int64_t>(now);
  auto* copy = new WifiCredential(*credential);
  if (xTaskCreate(syncTask, "BgTimeSync", 6144, copy, 1, &taskHandle) != pdPASS) {
    delete copy;
    taskHandle = nullptr;
  }
}

void stop() {
  if (!isRunning()) return;
  cancelRequested = true;
  for (int i = 0; i < 50 && isRunning(); i++) {
    delay(100);
  }
}

}  // namespace BackgroundTimeSync
