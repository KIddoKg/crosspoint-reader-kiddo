#include "BluetoothManager.h"
#include <Logging.h>
#include <algorithm>

void BluetoothManager::startScan(unsigned long durationMs) {
  if (isScanning) {
    LOG_ERR("BT", "Scan already in progress");
    return;
  }

  LOG_INF("BT", "Starting Bluetooth scan for %lu ms", durationMs);
  isScanning = true;
  scanStartTime = millis();
  clearScannedDevices();

  // TODO: Implement actual BLE scan using BLE library
  // For ESP32: Use BLEScan API
  // BLEScan* pBLEScan = BLEDevice::getScan();
  // pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  // pBLEScan->setActiveScan(true);
  // pBLEScan->start(durationMs, false);
}

void BluetoothManager::stopScan() {
  if (!isScanning) {
    return;
  }

  LOG_INF("BT", "Stopping Bluetooth scan");
  isScanning = false;

  // TODO: Implement actual BLE scan stop
  // BLEScan* pBLEScan = BLEDevice::getScan();
  // pBLEScan->stop();
}

unsigned long BluetoothManager::getScanProgress() const {
  if (!isScanning) {
    return 0;
  }
  // Default 10 second scan
  return (millis() - scanStartTime);
}

void BluetoothManager::pairDevice(const BluetoothDevice& device) {
  LOG_INF("BT", "Pairing device: %s (%s)", device.name.c_str(), device.address.c_str());

  // Check if already paired
  auto it = std::find_if(
      pairedDevices.begin(), pairedDevices.end(),
      [&device](const BluetoothDevice& d) { return d.address == device.address; });

  if (it != pairedDevices.end()) {
    LOG_ERR("BT", "Device already paired");
    return;
  }

  // Add to paired devices
  BluetoothDevice pairedDevice = device;
  pairedDevice.isPaired = true;
  pairedDevices.push_back(pairedDevice);

  LOG_INF("BT", "Device paired successfully");
  // TODO: Save paired devices to persistent storage
}

void BluetoothManager::unpairDevice(const std::string& address) {
  LOG_INF("BT", "Unpairing device: %s", address.c_str());

  auto it = std::find_if(pairedDevices.begin(), pairedDevices.end(),
                         [&address](const BluetoothDevice& d) {
                           return d.address == address;
                         });

  if (it != pairedDevices.end()) {
    pairedDevices.erase(it);
    LOG_INF("BT", "Device unpaired successfully");
  } else {
    LOG_ERR("BT", "Device not found in paired list");
  }

  // TODO: Save paired devices to persistent storage
}

void BluetoothManager::connectDevice(const std::string& address) {
  LOG_INF("BT", "Connecting to device: %s", address.c_str());

  // Find device in paired or scanned devices
  auto it = std::find_if(
      scannedDevices.begin(), scannedDevices.end(),
      [&address](const BluetoothDevice& d) { return d.address == address; });

  if (it != scannedDevices.end()) {
    connectedDevice = new BluetoothDevice(*it);
    LOG_INF("BT", "Device connected: %s", it->name.c_str());

    // TODO: Implement actual BLE connection
    // BLEClient* pClient = BLEDevice::createClient();
    // pClient->connect(address);
  } else {
    LOG_ERR("BT", "Device not found");
  }
}

void BluetoothManager::disconnectDevice() {
  if (connectedDevice == nullptr) {
    LOG_ERR("BT", "No device connected");
    return;
  }

  LOG_INF("BT", "Disconnecting from device: %s", connectedDevice->name.c_str());

  // TODO: Implement actual BLE disconnect
  // if (pClient != nullptr) {
  //   pClient->disconnect();
  // }

  delete connectedDevice;
  connectedDevice = nullptr;

  LOG_INF("BT", "Device disconnected");
}

void BluetoothManager::addDiscoveredDevice(const std::string& address,
                                          const std::string& name, int rssi) {
  // Check if already in list
  auto it = std::find_if(
      scannedDevices.begin(), scannedDevices.end(),
      [&address](const BluetoothDevice& d) { return d.address == address; });

  if (it != scannedDevices.end()) {
    // Update RSSI and last seen time
    it->rssi = rssi;
    it->lastSeen = time(nullptr);
  } else {
    // Add new device
    scannedDevices.push_back(BluetoothDevice(address, name, rssi));
  }

  LOG_DBG("BT", "Device discovered: %s [%s] RSSI: %d dBm", name.c_str(),
         address.c_str(), rssi);
}
