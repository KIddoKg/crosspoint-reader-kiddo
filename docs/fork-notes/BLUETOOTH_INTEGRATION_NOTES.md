# Bluetooth Integration Notes

## Current Status

### Completed Components:
1. **BluetoothManager.h/cpp** ✅
   - BLE device scanning (startScan, stopScan)
   - Device pairing/unpairing (pairDevice, unpairDevice)
   - Device connection management (connectDevice, disconnectDevice)
   - Device discovery tracking (addDiscoveredDevice)
   - Signal strength monitoring (RSSI analysis)
   - Singleton pattern for app-wide access

2. **FeaturesActivity Integration** ✅
   - TIME tab: Manual time setting with hour/minute/second adjustment
   - WEATHER tab: City selection with quick presets
   - BLUETOOTH tab: Skeleton structure for Bluetooth settings
   - Proper callback integration with onGoBack()

3. **main.cpp Integration** ✅
   - Included BluetoothManager header
   - Added onGoToBluetoothActivity() callback
   - Proper callback routing established

### BluetoothActivity Status:
Created but **NOT YET IMPLEMENTED** - requires extensive work:
- Current implementation uses incompatible GfxRenderer API calls
- Needs integration with theme system
- Needs proper MappedInputManager button handling
- Complex multi-state UI rendering

## Recommended Integration Path (Simplified)

Instead of a full standalone BluetoothActivity, integrate directly into FeaturesActivity:

### Option 1: Extend FeaturesActivity (RECOMMENDED)
- Add BLUETOOTH tab with simplified UI
- Buttons: "Scan Devices", "Paired List", "Clear Pairs"
- Show scan progress with basic status
- Reuse existing FeaturesActivity rendering system

### Option 2: Create ActivityWithSubactivity
- Follow SettingsActivity pattern
- Create BluetoothSubActivity
- Properly handle theme integration
- Full rendering capabilities

## TODO for Full Implementation:

1. **Remove/Simplify BluetoothActivity.cpp/h**
   - Current implementation incompatible with GfxRenderer API
   - Would require complete rewrite with theme system

2. **Add Bluetooth methods to FeaturesActivity**
   - handleBluetoothInput()
   - renderBluetoothTab()
   - startScanCallback()
   - Device list state management

3. **Implement actual BLE library calls in BluetoothManager**
   - Replace TODO comments with NimBLE/Arduino BLE API
   - Implement device discovery callbacks
   - Implement pairing flow
   - Implement connection establishment

4. **Test on hardware**
   - Serial connection to debug
   - Actual BLE device pairing
   - Connection stability
   - Memory usage validation

## Code Files:
- src/util/BluetoothManager.h/cpp ✅
- src/activities/settings/FeaturesActivity.h/cpp ✅
- src/activities/settings/BluetoothActivity.h/cpp ⚠️ (Needs rewrite)
- src/main.cpp ✅ (integration points added)

## Next Steps:
1. Simplify BluetoothActivity to work within existing theme system
2. Or extend FeaturesActivity.BLUETOOTH tab with full functionality
3. Implement actual ESP32 BLE library integration in BluetoothManager
4. Test compilation and on-device behavior
