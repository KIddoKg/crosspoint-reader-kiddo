# Bluetooth Integration - Implementation Status & Next Steps

## Summary

This document describes the Bluetooth component integration into the Xteink X4 e-reader firmware and the current implementation status.

## Completed Components

### 1. BluetoothManager (Utility Class) ✅
**Files**: `src/util/BluetoothManager.h`, `src/util/BluetoothManager.cpp`

**Purpose**: Core BLE management for device scanning, pairing, connection, and state tracking.

**Key Features**:
- Device scanning with configurable duration
- Device discovery with RSSI signal strength tracking
- Pairing/unpairing management
- Connection/disconnection handling
- Singleton pattern for app-wide access
- Proper logging and error handling

**Status**: Implementation complete with TODO placeholders for actual ESP32 BLE library integration

**Integration into main.cpp**: ✅
- Header included: `#include <util/BluetoothManager.h>`
- Accessible via: `BluetoothManager::getInstance()`

### 2. FeaturesActivity (Settings Hub) ✅ 
**Files**: `src/activities/settings/FeaturesActivity.h`, `src/activities/settings/FeaturesActivity.cpp`

**Purpose**: Multi-tab settings activity with Time, Weather, and Bluetooth configuration.

**Current Structure**:
- TIME tab: Manual time setting with hour/minute/second fields
- WEATHER tab: City selection with quick preset buttons (Hanoi, HCM, Da Nang, Can Tho)
- BLUETOOTH tab: Placeholder for Bluetooth settings UI

**Integration**: Called from HomeActivity settings menu

### 3. Main.cpp Activity Router ✅
**Changes**:
```cpp
// Added callback for BluetoothActivity
void onGoToBluetoothActivity() {
  exitActivity();
  enterNewActivity(new BluetoothActivity(renderer, mappedInputManager, onGoHome));
}
```

**Status**: Router integration point established

## Components In Progress

### BluetoothActivity (UI Layer) ⚠️
**Files**: `src/activities/settings/BluetoothActivity.h`, `src/activities/settings/BluetoothActivity.cpp`

**Status**: NEEDS REWRITE - API compatibility issues
- Current implementation uses incompatible GfxRenderer method calls
- Requires integration with existing theme/rendering system
- Button input handling needs adjustment for MappedInputManager patterns

**Current Issues**:
1. GfxRenderer doesn't have `fillScreen()`, `setTextColor()`, `println()` methods
2. Rendering needs to use theme system or proper GfxRenderer API
3. Button constants use HalGPIO, but input flow unclear
4. Complex state machine may be unnecessary

**Recommended Fix**: 
Choose ONE of these approaches:

**Option A: Simplify & Extend FeaturesActivity** (RECOMMENDED)
- Add Bluetooth configuration to existing FeaturesActivity
- Reuse time/weather tab infrastructure
- Simpler button handling already in place
- Less code duplication

**Option B: Create proper ActivityWithSubactivity**
- Follow SettingsActivity pattern
- Requires understanding of activity subactivity lifecycle
- More complex but more modular

**Option C: Full rewrite of BluetoothActivity**
- Study existing activity implementations (HomeActivity, ReaderActivity)
- Match rendering patterns exactly
- Implement theme-aware rendering

## Architecture Overview

```
main.cpp
├─ onGoHome() 
│  └─ new HomeActivity(...)
│     ├─ onGoToSettings()
│     │  └─ new SettingsActivity(...)
│     │     └─ onGoToFeaturesActivity()
│     │        └─ new FeaturesActivity(..., onGoBack)
│     │           └── BluetoothManager::getInstance() [TIME/WEATHER/BLUETOOTH tabs]
│     └─ onGoToBluetoothActivity() [DIRECT ACCESS]
│        └─ new BluetoothActivity(...)
│           └── BluetoothManager::getInstance()
└─ MappedInputManager
└─ GfxRenderer
```

## Bluetooth Manager Implementation Details

### Core Methods:

```cpp
// Scanning
void startScan(unsigned long durationMs);  // Begin BLE scan
void stopScan();                           // Stop active scan
unsigned long getScanProgress() const;     // Get scan progress (0-100ms)
bool isCurrentlyScanning() const;          // Check scan state

// Device Management
void pairDevice(const BluetoothDevice& device);           // Add to paired list
void unpairDevice(const std::string& address);           // Remove from paired
void connectDevice(const std::string& address);          // Establish connection
void disconnectDevice();                                 // Close connection

// Device Discovery
void addDiscoveredDevice(const std::string& address, 
                        const std::string& name, 
                        int rssi);                        // Called by BLE scan callback

// State Queries  
const std::vector<BluetoothDevice>& getScannedDevices() const;
const std::vector<BluetoothDevice>& getPairedDevices() const;
const BluetoothDevice* getConnectedDevice() const;
void clearPairedDevices();                  // Clear all paired devices
```

### BluetoothDevice Structure:
```cpp
struct BluetoothDevice {
  std::string address;      // MAC address
  std::string name;         // Device name
  int rssi;                 // Signal strength (-100 to 0 dBm)
  bool isPaired;            // Pairing state
  time_t lastSeen;          // Last discovery time
};
```

## Required ESP32 BLE Library Integration

The following TODO items in BluetoothManager.cpp need implementation:

1. **startScan()** - Line ~40
   - Initialize BLE scan using NimBLE or Arduino BLE library
   - Set scan callback for device discovery
   - Configure scan parameters (active vs passive, duration)

2. **stopScan()** - Line ~50
   - Stop active BLE scan
   - Clean up scan resources

3. **connectDevice()** - Line ~95
   - Create BLE client connection
   - Establish GATT connection
   - Handle connection callbacks

4. **disconnectDevice()** - Line ~110
   - Properly close BLE connection
   - Cleanup client resources

## Testing Checklist

- [ ] Code compiles without errors
- [ ] BluetoothManager singleton initializes
- [ ] Activity routing works (onGoToBluetoothActivity)
- [ ] FeaturesActivity BLUETOOTH tab accessible
- [ ] Button input recognized by activity
- [ ] Rendering displays on e-ink screen
- [ ] BLE scan initiates and finds devices
- [ ] Device pairing flow works
- [ ] Connection establishment successful
- [ ] Paired devices persist across reboots

## Build Instructions

```bash
# From project root
cd /Volumes/Work/SyncData/backup_data/Program/Life/Work/WorkAtHome/C/crosspoint-reader-vi

# Build with PlatformIO
pio run

# Upload to board
pio run --target upload
```

## Known Limitations

1. **BLE Library Not Integrated**: All actual BLE operations marked as TODO
2. **Rendering API**: BluetoothActivity needs rewrite for proper rendering
3. **Button Handling**: Simplified - may need ButtonNavigator pattern
4. **Persistence**: Paired devices not saved to storage yet
5. **Error Handling**: Minimal error recovery in BLE operations
6. **Memory**: Large device lists could cause issues on ESP32

## Next Priority Actions

1. **Decide on UI approach** (Extend FeaturesActivity vs. Rewrite BluetoothActivity)
2. **Fix compilation errors** in BluetoothActivity/rendering
3. **Implement actual BLE library calls** in BluetoothManager
4. **Test on hardware** with ESP32-C3-DevKitM-1 board
5. **Optimize memory usage** for device lists
6. **Add persistent storage** for paired devices

## Related Files

- Activity base class: `src/activities/Activity.h`
- MappedInputManager: `src/MappedInputManager.h`
- HomeActivity example: `src/activities/home/HomeActivity.h`
- SettingsActivity example: `src/activities/settings/SettingsActivity.h`
- GfxRenderer: `lib/GfxRenderer/GfxRenderer.h`
- HalGPIO: `lib/hal/HalGPIO.h`

## Support & Documentation

For more information on the new UI theme and architecture:
- See: `IMPLEMENTATION_NEW_THEME.md`
- See: `UI_ARCHITECTURE_EXPLANATION.md`
- See: `UI_TESTING_GUIDE.md`
