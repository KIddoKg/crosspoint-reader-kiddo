#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "activities/ActivityWithSubactivity.h"
#include "util/ButtonNavigator.h"
#include "../network/WifiSelectionActivity.h"

class ReloadWiFiActivity final : public ActivityWithSubactivity {
  ButtonNavigator buttonNavigator;

  WifiSelectionState state = WifiSelectionState::SCANNING;
  size_t selectedNetworkIndex = 0;
  std::vector<WifiNetworkInfo> networks;
  const std::function<void()> onComplete;

  std::string selectedSSID;
  bool selectedRequiresPassword = false;
  std::string connectedIP;
  std::string connectionError;
  std::string enteredPassword;
  std::string cachedMacAddress;
  bool usedSavedPassword = false;
  bool syncComplete = false; // Thêm cờ trạng thái đồng bộ

  int savePromptSelection = 0;
  int forgetPromptSelection = 0;

  static constexpr unsigned long CONNECTION_TIMEOUT_MS = 15000;
  unsigned long connectionStartTime = 0;

  void renderNetworkList() const;
  void renderConnecting() const;
  void renderConnected() const;
  void renderSavePrompt() const;
  void renderConnectionFailed() const;
  void renderForgetPrompt() const;

  void startWifiScan();
  void processWifiScanResults();
  void selectNetwork(int index);
  void attemptConnection();
  void checkConnectionStatus();
  void syncDateTimeAndWeather();
  std::string getSignalStrengthIndicator(int32_t rssi) const;

 public:
  explicit ReloadWiFiActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                 const std::function<void()>& onComplete)
      : ActivityWithSubactivity("ReloadWiFi", renderer, mappedInput),
        onComplete(onComplete) {}
  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(Activity::RenderLock&&) override;
};
