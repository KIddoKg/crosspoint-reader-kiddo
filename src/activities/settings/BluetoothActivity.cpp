// // #include "BluetoothActivity.h"
// // #include <Logging.h>
// // #include <BluetoothSerial.h>
// // #include <fontIds.h>

// // // Global Bluetooth serial instance
// // BluetoothSerial SerialBT;

// // void BluetoothActivity::onEnter() {
// //   LOG_DBG("BT", "Entering Bluetooth Activity");
// //   startBluetoothScan();
// //   requestUpdate();
// // }

// // void BluetoothActivity::onExit() {
// //   LOG_DBG("BT", "Exiting Bluetooth Activity");
// //   if (state == BluetoothActivityState::CONNECTED) {
// //     disconnectDevice();
// //   }
// //   SerialBT.end();
// // }

// // void BluetoothActivity::loop() {
// //   // Handle scanning state
// //   if (state == BluetoothActivityState::SCANNING) {
// //     processBluetoothScanResults();
// //   }

// //   // Handle device list navigation
// //   if (state == BluetoothActivityState::DEVICE_LIST) {
// //     if (mappedInput.wasPressed(MappedInputManager::Button::Up)) {
// //       if (selectedDevice > 0) selectedDevice--;
// //       requestUpdate();
// //     } else if (mappedInput.wasPressed(MappedInputManager::Button::Down)) {
// //       if (selectedDevice < (int)devices.size() - 1) selectedDevice++;
// //       requestUpdate();
// //     }

// //     // Confirm: attempt to connect to selected device
// //     if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
// //       selectDevice(selectedDevice);
// //       requestUpdate();
// //     }
// //   }

// //   // Handle connection state
// //   if (state == BluetoothActivityState::CONNECTING) {
// //     checkConnectionStatus();
// //   }

// //   // Handle connected state
// //   if (state == BluetoothActivityState::CONNECTED) {
// //     if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
// //       disconnectDevice();
// //       state = BluetoothActivityState::DEVICE_LIST;
// //       requestUpdate();
// //     }
// //   }

// //   // Handle error state
// //   if (state == BluetoothActivityState::ERROR) {
// //     if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
// //       state = BluetoothActivityState::DEVICE_LIST;
// //       errorMessage.clear();
// //       requestUpdate();
// //     }
// //   }

// //   // Back button: exit activity
// //   if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
// //     if (state == BluetoothActivityState::DEVICE_LIST || state == BluetoothActivityState::SCANNING) {
// //       onDone(state == BluetoothActivityState::CONNECTED);
// //     }
// //   }
// // }

// // void BluetoothActivity::startBluetoothScan() {
// //   LOG_DBG("BT", "Starting Bluetooth scan");
// //   state = BluetoothActivityState::SCANNING;
// //   devices.clear();
// //   selectedDevice = 0;
// //   scanStartTime = millis();

// //   // Initialize Bluetooth
// //   if (!SerialBT.begin("CrossPoint Reader")) {
// //     LOG_ERR("BT", "Failed to initialize Bluetooth");
// //     state = BluetoothActivityState::ERROR;
// //     errorMessage = "Failed to initialize Bluetooth";
// //   }
// // }

// // void BluetoothActivity::processBluetoothScanResults() {
// //   // Scan timeout check
// //   if (millis() - scanStartTime > 10000) {  // 10 seconds timeout
// //     LOG_DBG("BT", "Bluetooth scan completed with %d devices found", devices.size());
// //     state = BluetoothActivityState::DEVICE_LIST;
// //     requestUpdate();
// //     return;
// //   }

// //   // TODO: Implement actual Bluetooth discovery using BTAddress scanning
// //   // This is a placeholder - real implementation would use BTDiscoveryRequest or similar
// //   // For now, we'll populate with dummy data or known paired devices

// //   // Example: scan paired devices (if any exist)
// //   // In a real implementation, you would use esp_bt_gap_start_discovery() or similar

// //   requestUpdate();
// // }

// // void BluetoothActivity::selectDevice(int index) {
// //   if (index < 0 || index >= (int)devices.size()) return;

// //   LOG_DBG("BT", "Selected device: %s (%s)", devices[index].name.c_str(), devices[index].address.c_str());
// //   attemptConnection();
// // }

// // void BluetoothActivity::attemptConnection() {
// //   if (selectedDevice < 0 || selectedDevice >= (int)devices.size()) return;

// //   state = BluetoothActivityState::CONNECTING;
// //   const BluetoothDeviceInfo& device = devices[selectedDevice];

// //   LOG_DBG("BT", "Attempting to connect to %s", device.address.c_str());

// //   // TODO: Implement actual Bluetooth connection using BTAddress
// //   // Example (pseudo-code):
// //   // BluetoothAddress addr;
// //   // addr.fromString(device.address);
// //   // if (SerialBT.connect(addr)) {
// //   //   state = BluetoothActivityState::CONNECTED;
// //   //   connectedDeviceName = device.name;
// //   // } else {
// //   //   state = BluetoothActivityState::ERROR;
// //   //   errorMessage = "Failed to connect to device";
// //   // }

// //   requestUpdate();
// // }

// // void BluetoothActivity::checkConnectionStatus() {
// //   // TODO: Check if connection is established
// //   // For now, assume connection succeeded after a short delay
// //   if (connectionStartTime == 0) {
// //     connectionStartTime = millis();
// //   }

// //   if (millis() - connectionStartTime > 2000) {  // 2 second timeout
// //     connectionStartTime = 0;
// //     state = BluetoothActivityState::CONNECTED;
// //     connectedDeviceName = devices[selectedDevice].name;
// //     LOG_DBG("BT", "Connected to %s", connectedDeviceName.c_str());
// //     requestUpdate();
// //   }
// // }

// // void BluetoothActivity::disconnectDevice() {
// //   LOG_DBG("BT", "Disconnecting from %s", connectedDeviceName.c_str());
// //   SerialBT.disconnect();
//   connectedDeviceName.clear();
//   state = BluetoothActivityState::DEVICE_LIST;
// }

// void BluetoothActivity::render(Activity::RenderLock&&) {
//   renderer.clearScreen();
//   statusBar.render();

//   int y = 25 + 10;

//   switch (state) {
//     case BluetoothActivityState::SCANNING: {
//       renderer.drawText(UI_12_FONT_ID, 10, y, "Bluetooth", true);
//       y += renderer.getLineHeight(UI_12_FONT_ID) + 10;
//       renderer.drawText(UI_12_FONT_ID, 10, y, "Scanning for devices...", true);
//       y += renderer.getLineHeight(UI_12_FONT_ID) + 6;
//       renderer.drawText(UI_10_FONT_ID, 10, y, "Please wait...", true);
//       break;
//     }

//     case BluetoothActivityState::DEVICE_LIST: {
//       renderer.drawText(UI_12_FONT_ID, 10, y, "Available Devices", true);
//       y += renderer.getLineHeight(UI_12_FONT_ID) + 10;

//       if (devices.empty()) {
//         renderer.drawText(UI_12_FONT_ID, 10, y, "No devices found", true);
//       } else {
//         for (size_t i = 0; i < devices.size(); i++) {
//           bool isSelected = (int)i == selectedDevice;
          
//           if (isSelected) {
//             renderer.drawRect(5, y - 2, 220, 30, true);  // Selection highlight
//           }

//           std::string deviceLabel = devices[i].name + " ";
//           if (devices[i].isPaired) deviceLabel += "[Paired]";
          
//           renderer.drawText(UI_10_FONT_ID, 15, y, deviceLabel.c_str(), isSelected);
//           y += renderer.getLineHeight(UI_10_FONT_ID) + 8;
//         }
//       }
//       break;
//     }

//     case BluetoothActivityState::CONNECTING: {
//       renderer.drawText(UI_12_FONT_ID, 10, y, "Connecting...", true);
//       y += renderer.getLineHeight(UI_12_FONT_ID) + 10;
//       renderer.drawText(UI_12_FONT_ID, 10, y, devices[selectedDevice].name.c_str(), true);
//       break;
//     }

//     case BluetoothActivityState::CONNECTED: {
//       renderer.drawText(UI_12_FONT_ID, 10, y, "Connected", true);
//       y += renderer.getLineHeight(UI_12_FONT_ID) + 10;
//       renderer.drawText(UI_12_FONT_ID, 10, y, connectedDeviceName.c_str(), true);
//       y += renderer.getLineHeight(UI_12_FONT_ID) + 10;
//       renderer.drawText(UI_10_FONT_ID, 10, y, "Press Back to disconnect", true);
//       break;
//     }

//     case BluetoothActivityState::ERROR: {
//       renderer.drawText(UI_12_FONT_ID, 10, y, "Error", true);
//       y += renderer.getLineHeight(UI_12_FONT_ID) + 10;
//       renderer.drawText(UI_12_FONT_ID, 10, y, errorMessage.c_str(), true);
//       y += renderer.getLineHeight(UI_12_FONT_ID) + 10;
//       renderer.drawText(UI_10_FONT_ID, 10, y, "Press Back to return", true);
//       break;
//     }
//   }

//   renderer.displayBuffer();
// }
