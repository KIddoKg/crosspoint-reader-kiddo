#pragma once
#include <functional>
#include <string>
#include <vector>

#include "../Activity.h"
#include "util/ButtonNavigator.h"

/**
 * BluetoothActivity allows scanning and connecting to Bluetooth devices
 */
class BluetoothActivity final : public Activity {
  ButtonNavigator buttonNavigator;
  const std::function<void()> onGoHome;

  enum class BluetoothState { SCANNING, CONNECTING, CONNECTED, DISCONNECTED, ERROR };

  BluetoothState state = BluetoothState::DISCONNECTED;
  std::vector<std::string> availableDevices;
  int selectedDeviceIndex = 0;
  std::string statusMessage;
  std::string connectedDeviceName;
  unsigned long lastScanTime = 0;
  static constexpr unsigned long SCAN_INTERVAL = 30000;  // Rescan every 30 seconds

  void scanDevices();
  void connectToDevice(const std::string& deviceName);
  void disconnectDevice();

 public:
  explicit BluetoothActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                            const std::function<void()>& onGoHome)
      : Activity("Bluetooth", renderer, mappedInput), onGoHome(onGoHome) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(Activity::RenderLock&&) override;
};
