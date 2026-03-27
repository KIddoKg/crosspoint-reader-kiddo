// #pragma once
// #include <activities/ActivityWithSubactivity.h>
// #include <components/StatusBar.h>
// #include <vector>
// #include <string>
// #include <functional>

// struct BluetoothDeviceInfo {
//   std::string name;
//   std::string address;
//   int rssi;
//   bool isPaired;
// };

// enum class BluetoothActivityState {
//   SCANNING,      // Scanning for devices
//   DEVICE_LIST,   // Showing list of found devices
//   CONNECTING,    // Attempting to connect
//   CONNECTED,     // Successfully connected
//   ERROR          // Connection error
// };

// class BluetoothActivity final : public ActivityWithSubactivity {
//  private:
//   StatusBar statusBar;
//   BluetoothActivityState state = BluetoothActivityState::SCANNING;
//   std::vector<BluetoothDeviceInfo> devices;
//   int selectedDevice = 0;
//   std::string connectedDeviceName;
//   std::string errorMessage;
//   std::function<void(bool)> onDone;
//   unsigned long scanStartTime = 0;
//   unsigned long connectionStartTime = 0;

//   // Helper methods
//   void startBluetoothScan();
//   void processBluetoothScanResults();
//   void selectDevice(int index);
//   void attemptConnection();
//   void checkConnectionStatus();
//   void disconnectDevice();

//  public:
//   explicit BluetoothActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
//                              std::function<void(bool)> onDone)
//       : ActivityWithSubactivity("Bluetooth", renderer, mappedInput), statusBar(renderer), onDone(onDone) {}

//   void onEnter() override;
//   void onExit() override;
//   void loop() override;
//   void render(Activity::RenderLock&&) override;
// };
