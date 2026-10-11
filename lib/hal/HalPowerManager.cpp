#include "HalPowerManager.h"

#include <Logging.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <driver/gpio.h>

#include <cassert>

#include "HalGPIO.h"
#include "../../src/CrossPointSettings.h"

HalPowerManager powerManager;  // Singleton instance

// GPIO13 (the flash SPIWP pad, unused in DIO flash mode) drives the battery-latch MOSFET: high keeps the battery
// connected, low powers the device off.
static constexpr gpio_num_t GPIO_BATTERY_LATCH = GPIO_NUM_13;

void HalPowerManager::begin() {
  pinMode(BAT_GPIO0, INPUT);
  normalFreq = getCpuFrequencyMhz();
  modeMutex = xSemaphoreCreateMutex();
  assert(modeMutex != nullptr);
}

void HalPowerManager::setPowerSaving(bool enabled) {
  if (normalFreq <= 0) {
    return;  // invalid state
  }

  auto wifiMode = WiFi.getMode();
  if (wifiMode != WIFI_MODE_NULL) {
    // Wifi is active, force disabling power saving
    enabled = false;
  }

  // Note: We don't use mutex here to avoid too much overhead,
  // it's not very important if we read a slightly stale value for currentLockMode
  const LockMode mode = currentLockMode;

  if (mode == None && enabled && !isLowPower) {
    LOG_DBG("PWR", "Going to low-power mode");
    if (!setCpuFrequencyMhz(LOW_POWER_FREQ)) {
      LOG_DBG("PWR", "Failed to set CPU frequency = %d MHz", LOW_POWER_FREQ);
      return;
    }
    isLowPower = true;

  } else if ((!enabled || mode != None) && isLowPower) {
    LOG_DBG("PWR", "Restoring normal CPU frequency");
    if (!setCpuFrequencyMhz(normalFreq)) {
      LOG_DBG("PWR", "Failed to set CPU frequency = %d MHz", normalFreq);
      return;
    }
    isLowPower = false;
  }

  // Otherwise, no change needed
}

void HalPowerManager::startDeepSleep(HalGPIO& gpio) const {
  // lightSleep() pad-holds the battery latch HIGH while running, so the latch level must be set explicitly here.
  gpio_hold_dis(GPIO_BATTERY_LATCH);
  gpio_set_direction(GPIO_BATTERY_LATCH, GPIO_MODE_OUTPUT);
  if (SETTINGS.timeTrackingInSleep) {
    // Keep the battery connected through deep sleep so the RTC keeps counting (costs battery: the board stays powered)
    gpio_set_level(GPIO_BATTERY_LATCH, 1);
    LOG_INF("PWR", "RTC domain will be held ON during deep sleep");
  } else {
    // Drive the latch low: on battery this powers the device off (~20uA); on USB power the GPIO wake still works.
    // RTC time is lost until the next sync.
    gpio_set_level(GPIO_BATTERY_LATCH, 0);
    LOG_INF("PWR", "RTC domain will be powered off during deep sleep");
  }
  gpio_hold_en(GPIO_BATTERY_LATCH);
  gpio_deep_sleep_hold_en();

  // Ensure that the power button has been released to avoid immediately turning back on if you're holding it
  while (gpio.isPressed(HalGPIO::BTN_POWER)) {
    delay(50);
    gpio.update();
  }
  // Arm the wakeup trigger *after* the button is released
  esp_deep_sleep_enable_gpio_wakeup(1ULL << InputManager::POWER_BUTTON_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
  // Enter Deep Sleep
  esp_deep_sleep_start();
}

bool HalPowerManager::lightSleep(const HalGPIO& gpio) const {
  // A performance Lock means a render is mid-flight; light sleep freezes the whole chip and would stall it.
  // Read without the mutex like setPowerSaving(): a stale value costs at most one 50 ms slice.
  if (currentLockMode != None) {
    return false;
  }
  // Light sleep drops a WiFi association and kills an enumerated USB-CDC link.
  if (WiFi.getMode() != WIFI_MODE_NULL || gpio.isUsbConnected()) {
    return false;
  }

  // By default the C3 switches GPIO pads to their "sleep" configuration during light sleep, which lets the e-ink and
  // SD lines (RST, CS, DC, SPI) float: the panel controller then resets mid-refresh and stays BUSY forever (frozen
  // screen). Keep these pads in their active configuration through light sleep.
  static bool padsPinned = false;
  if (!padsPinned) {
    for (const int pin : {EPD_SCLK, EPD_MOSI, EPD_CS, EPD_DC, EPD_RST, EPD_BUSY, 7 /* SD MISO */, 12 /* SD CS */}) {
      gpio_sleep_sel_dis(static_cast<gpio_num_t>(pin));
    }
    padsPinned = true;
  }

  // Timer wake keeps the button poll cadence of the delay() this replaces: the front/side buttons are ADC
  // resistor-ladder inputs that cannot wake the chip, so they must be polled. The power button is a real GPIO and
  // is also armed (level wake at its pressed level) so taps shorter than a slice are not dropped; update()'s debounce
  // still decides whether it was a real press.
  esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(LIGHT_SLEEP_SLICE_MS) * 1000ULL);
  const auto powerPin = static_cast<gpio_num_t>(InputManager::POWER_BUTTON_PIN);
  gpio_wakeup_enable(powerPin, GPIO_INTR_LOW_LEVEL);
  esp_sleep_enable_gpio_wakeup();

  // The IDF flash-leakage workaround pulls the unused SPIWP pad low on light-sleep entry, which on this board releases
  // the battery latch and hard-powers the device off. Drive the latch high and pad-hold it; the hold stays enabled
  // while running and is only released in startDeepSleep(). Level before direction so the pad never glitches low.
  gpio_set_level(GPIO_BATTERY_LATCH, 1);
  gpio_set_direction(GPIO_BATTERY_LATCH, GPIO_MODE_OUTPUT);
  gpio_hold_en(GPIO_BATTERY_LATCH);

  const esp_err_t err = esp_light_sleep_start();

  // Disarm immediately: an armed timer wake would carry over into startDeepSleep(). gpio_wakeup_disable() leaves the
  // level interrupt type behind, which can livelock a later GPIO ISR service, so clear it explicitly.
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  gpio_wakeup_disable(powerPin);
  gpio_set_intr_type(powerPin, GPIO_INTR_DISABLE);

  if (err != ESP_OK) {
    LOG_DBG("PWR", "Light sleep rejected: %d", static_cast<int>(err));
    return false;
  }
  return true;
}

uint16_t HalPowerManager::getBatteryPercentage() const {
  static const BatteryMonitor battery = BatteryMonitor(BAT_GPIO0);
  return battery.readPercentage();
}

HalPowerManager::Lock::Lock() {
  xSemaphoreTake(powerManager.modeMutex, portMAX_DELAY);
  // Current limitation: only one lock at a time
  if (powerManager.currentLockMode != None) {
    LOG_ERR("PWR", "Lock already held, ignore");
    valid = false;
  } else {
    powerManager.currentLockMode = NormalSpeed;
    valid = true;
  }
  xSemaphoreGive(powerManager.modeMutex);
  if (valid) {
    // Immediately restore normal CPU frequency if currently in low-power mode
    powerManager.setPowerSaving(false);
  }
}

HalPowerManager::Lock::~Lock() {
  xSemaphoreTake(powerManager.modeMutex, portMAX_DELAY);
  if (valid) {
    powerManager.currentLockMode = None;
  }
  xSemaphoreGive(powerManager.modeMutex);
}
