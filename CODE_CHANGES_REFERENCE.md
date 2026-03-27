# Code Changes Reference

## 1. NewGridHomeActivity.h

### Constructor Changes
```cpp
// BEFORE
explicit NewGridHomeActivity(
    GfxRenderer& renderer, MappedInputManager& mappedInput,
    const std::function<void(const std::string&)>& onSelectBook,
    const std::function<void()>& onMyLibraryOpen,
    const std::function<void()>& onRecentsOpen,
    const std::function<void()>& onSettingsOpen,
    const std::function<void()>& onFileTransferOpen,
    const std::function<void()>& onFeaturesOpen,
    const std::function<void()>& onBluetoothOpen)
    : Activity("NewGridHome", renderer, mappedInput),
      statusBar(renderer),
      gridMenu(renderer),
      timeManager(TimeManager::getInstance()),
      weatherManager(WeatherManager::getInstance()),
      onSelectBook(onSelectBook),
      onMyLibraryOpen(onMyLibraryOpen),
      onRecentsOpen(onRecentsOpen),
      onSettingsOpen(onSettingsOpen),
      onFileTransferOpen(onFileTransferOpen),
      onFeaturesOpen(onFeaturesOpen),
      onBluetoothOpen(onBluetoothOpen) {}

// AFTER
explicit NewGridHomeActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("NewGridHome", renderer, mappedInput),
      statusBar(renderer),
      gridMenu(renderer),
      timeManager(TimeManager::getInstance()),
      weatherManager(WeatherManager::getInstance()) {}
```

### Removed Members
```cpp
// REMOVED from private section
const std::function<void(const std::string&)> onSelectBook;
const std::function<void()> onMyLibraryOpen;
const std::function<void()> onRecentsOpen;
const std::function<void()> onSettingsOpen;
const std::function<void()> onFileTransferOpen;
const std::function<void()> onFeaturesOpen;
const std::function<void()> onBluetoothOpen;
```

---

## 2. NewGridHomeActivity.cpp

### Includes Added
```cpp
// ADDED
#include "../settings/FeaturesActivity.h"
#include <activities/ActivityManager.h>
```

### handleMenuSelection() Implementation
```cpp
// BEFORE
void NewGridHomeActivity::handleMenuSelection() {
  int selectedIndex = gridMenu.getSelectedIndex();
  LOG_INF("GRID_HOME", "Menu item selected: %d", selectedIndex);

  switch (selectedIndex) {
    case 0:  // My Library
      onMyLibraryOpen();
      break;
    case 1:  // Recent Books
      onRecentsOpen();
      break;
    case 2:  // Settings
      onSettingsOpen();
      break;
    case 3:  // File Transfer
      onFileTransferOpen();
      break;
    case 4:  // Features (Time/Weather Settings)
      onFeaturesOpen();
      break;
    case 5:  // Bluetooth
      onBluetoothOpen();
      break;
    default:
      LOG_ERR("GRID_HOME", "Unknown menu item: %d", selectedIndex);
      break;
  }
}

// AFTER
void NewGridHomeActivity::handleMenuSelection() {
  int selectedIndex = gridMenu.getSelectedIndex();
  LOG_INF("GRID_HOME", "Menu item selected: %d", selectedIndex);

  ActivityManager& activityMgr = ActivityManager::getInstance();

  switch (selectedIndex) {
    case 0:  // My Library
      LOG_INF("GRID_HOME", "Opening My Library");
      // TODO: Navigate to My Library activity
      break;
    case 1:  // Recent Books
      LOG_INF("GRID_HOME", "Opening Recent Books");
      // TODO: Navigate to Recent Books activity
      break;
    case 2:  // Settings
      LOG_INF("GRID_HOME", "Opening Settings");
      // TODO: Navigate to Settings activity
      break;
    case 3:  // File Transfer
      LOG_INF("GRID_HOME", "Opening File Transfer");
      // TODO: Navigate to File Transfer activity
      break;
    case 4:  // Features (Time/Weather Settings)
      LOG_INF("GRID_HOME", "Opening Features");
      activityMgr.enterNewActivity(new FeaturesActivity(
          renderer, mappedInput,
          [&activityMgr]() { activityMgr.pop(); }
      ));
      break;
    case 5:  // Bluetooth
      LOG_INF("GRID_HOME", "Opening Bluetooth");
      // TODO: Navigate to Bluetooth activity
      break;
    default:
      LOG_ERR("GRID_HOME", "Unknown menu item: %d", selectedIndex);
      break;
  }
}
```

---

## 3. main.cpp

### Include Added
```cpp
// ADDED AFTER existing includes
#include "activities/home/NewGridHomeActivity.h"
```

### onGoHome() Function Updated
```cpp
// BEFORE
void onGoHome() {
  exitActivity();
  enterNewActivity(new HomeActivity(renderer, mappedInputManager, onGoToReader, onGoToMyLibrary, onGoToRecentBooks,
                                    onGoToSettings, onGoToFileTransfer, onGoToBrowser));
}

// AFTER
void onGoHome() {
  exitActivity();
  enterNewActivity(new NewGridHomeActivity(renderer, mappedInputManager));
}
```

---

## Key Technical Details

### FeaturesActivity Constructor Signature
```cpp
explicit FeaturesActivity(GfxRenderer& renderer,
                         MappedInputManager& mappedInput,
                         const std::function<void()>& onGoBack)
```

### Features Enum (in FeaturesActivity)
```cpp
enum FeatureTab { TIME, WEATHER, BLUETOOTH, COUNT };
```

### Features Accessible
The FeaturesActivity provides:
- Time settings tab: Adjust hour, minute, second
- Weather settings tab: Set city for weather updates
- Bluetooth tab: Bluetooth device management
- Full navigation with Back button support

---

## ActivityManager Usage Pattern

```cpp
ActivityManager& activityMgr = ActivityManager::getInstance();

// Push new activity onto stack
activityMgr.enterNewActivity(new FeaturesActivity(
    renderer, mappedInput,
    [&activityMgr]() { activityMgr.pop(); }  // Back callback
));

// Or pop to previous activity
activityMgr.pop();
```

This pattern allows Activities to manage the activity stack directly without needing callbacks passed through constructors.
