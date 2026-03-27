#pragma once
#include <string>
#include <vector>
#include <ctime>

struct BluetoothDevice {
  std::string address;
  std::string name;
  int rssi;  // Signal strength
  bool isPaired;
  time_t lastSeen;

  BluetoothDevice() : rssi(-100), isPaired(false), lastSeen(0) {}

  BluetoothDevice(const std::string& addr, const std::string& n, int signal)
      : address(addr), name(n), rssi(signal), isPaired(false), lastSeen(time(nullptr)) {}
};

class BluetoothManager {
 private:
  std::vector<BluetoothDevice> scannedDevices;
  std::vector<BluetoothDevice> pairedDevices;
  BluetoothDevice* connectedDevice = nullptr;
  bool isScanning = false;
  unsigned long scanStartTime = 0;

  BluetoothManager() = default;

 public:
  static BluetoothManager& getInstance() {
    static BluetoothManager instance;
    return instance;
  }

  // Scanning
  void startScan(unsigned long durationMs = 10000);
  void stopScan();
  bool isCurrentlyScanning() const { return isScanning; }
  unsigned long getScanProgress() const;

  // Device management
  const std::vector<BluetoothDevice>& getScannedDevices() const {
    return scannedDevices;
  }
  const std::vector<BluetoothDevice>& getPairedDevices() const {
    return pairedDevices;
  }

  // Pairing & Connection
  void pairDevice(const BluetoothDevice& device);
  void unpairDevice(const std::string& address);
  void connectDevice(const std::string& address);
  void disconnectDevice();

  BluetoothDevice* getConnectedDevice() const { return connectedDevice; }

  // Add discovered device (called by scan callback)
  void addDiscoveredDevice(const std::string& address, const std::string& name,
                          int rssi);

  void clearScannedDevices() { scannedDevices.clear(); }
};
