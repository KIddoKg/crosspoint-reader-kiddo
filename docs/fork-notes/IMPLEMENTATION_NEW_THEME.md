# 🎨 Implementation Guide: New Theme (Grid-Based Home)

## 📋 Tổng Quan

Theme mới này sẽ thay thế HomeActivity hiện tại với layout grid-based:

```
┌─────────────────────────────────┐
│ Time | Temp | Battery           │  ← Header (Status bar)
├─────────────────────────────────┤
│  Recent Books (2 covers)        │  ← Recent books section
├─────────────────────────────────┤
│  [My Lib] [Recent] [Settings]   │  ← Grid menu 3x2
│  [Files]  [Features] [Bluetooth]│
└─────────────────────────────────┘
```

---

## 🏗️ Cấu Trúc Thư Mục Cần Tạo

```
src/
├── activities/home/
│   ├── HomeActivity.h (hiện tại)
│   ├── NewGridHomeActivity.h  ← TẠO MỚI
│   ├── NewGridHomeActivity.cpp ← TẠO MỚI
│   └── ...
├── components/
│   ├── StatusBar.h ← TẠO MỚI (time, temp, battery)
│   ├── StatusBar.cpp ← TẠO MỚI
│   ├── GridMenu.h ← TẠO MỚI (3x2 grid buttons)
│   └── GridMenu.cpp ← TẠO MỚI
└── util/
    ├── TimeManager.h ← TẠO MỚI (quản lý ngày giờ)
    ├── TimeManager.cpp ← TẠO MỚI
    ├── WeatherManager.h ← TẠO MỚI (quản lý thời tiết)
    └── WeatherManager.cpp ← TẠO MỚI
```

---

## 📝 Bước 1: Tạo StatusBar Component

File: `src/components/StatusBar.h`

```cpp
#pragma once
#include <GfxRenderer.h>
#include <ctime>

class StatusBar {
 private:
  GfxRenderer& renderer;
  int y = 0;  // Y position (top of screen)
  int height = 30;  // Status bar height

 public:
  explicit StatusBar(GfxRenderer& renderer) : renderer(renderer) {}

  void render();

 private:
  std::string getTimeString();
  std::string getTemperatureString();
  int getBatteryPercentage();
  void renderBattery(int x, int y, int percent);
};
```

File: `src/components/StatusBar.cpp`

```cpp
#include "StatusBar.h"
#include <HalDisplay.h>

void StatusBar::render() {
  // Draw background bar
  renderer.drawFilledRect(0, y, HalDisplay::WIDTH, height, EpdColor::WHITE);
  
  // Draw border
  renderer.drawRect(0, y, HalDisplay::WIDTH, height, EpdColor::BLACK);

  // Left: Time
  std::string timeStr = getTimeString();
  renderer.drawString(10, y + 8, timeStr.c_str(), EpdFontFamily::NORMAL, 10);

  // Middle: Temperature
  std::string tempStr = getTemperatureString();
  int centerX = HalDisplay::WIDTH / 2;
  renderer.drawString(centerX - 20, y + 8, tempStr.c_str(), EpdFontFamily::NORMAL, 10);

  // Right: Battery
  int battPercent = getBatteryPercentage();
  renderBattery(HalDisplay::WIDTH - 50, y + 5, battPercent);
}

std::string StatusBar::getTimeString() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%H:%M", timeinfo);
  return std::string(buffer);
}

std::string StatusBar::getTemperatureString() {
  // TODO: Integrate with WeatherManager
  return "25°C";
}

int StatusBar::getBatteryPercentage() {
  // TODO: Integrate with BatteryMonitor from HAL
  return 85;  // Placeholder
}

void StatusBar::renderBattery(int x, int y, int percent) {
  // Draw battery outline: ▌ shape
  renderer.drawRect(x, y, 30, 15, EpdColor::BLACK);
  
  // Draw fill based on percentage
  int fillWidth = (30 * percent) / 100;
  if (fillWidth > 0) {
    renderer.drawFilledRect(x + 1, y + 1, fillWidth - 1, 13, EpdColor::BLACK);
  }

  // Draw percentage text
  char percentStr[5];
  sprintf(percentStr, "%d%%", percent);
  renderer.drawString(x - 20, y + 2, percentStr, EpdFontFamily::NORMAL, 8);
}
```

---

## 🎮 Bước 2: Tạo GridMenu Component

File: `src/components/GridMenu.h`

```cpp
#pragma once
#include <GfxRenderer.h>
#include <vector>
#include <string>

class GridMenu {
 public:
  struct MenuItem {
    std::string label;
    int x, y, width, height;
    bool isSelected = false;
  };

 private:
  GfxRenderer& renderer;
  std::vector<MenuItem> items;
  int selectedIndex = 0;
  int rows = 2, cols = 3;  // 2 rows, 3 columns
  int startY = 100;  // Below status bar and recent books
  int itemWidth = 80;
  int itemHeight = 60;
  int gap = 10;

 public:
  explicit GridMenu(GfxRenderer& renderer) : renderer(renderer) {
    initializeGrid();
  }

  void initializeGrid();
  void render();
  void selectNext();
  void selectPrevious();
  void selectUp();
  void selectDown();
  int getSelectedIndex() const { return selectedIndex; }
  const MenuItem& getSelectedItem() const { return items[selectedIndex]; }

 private:
  void calculatePositions();
};
```

File: `src/components/GridMenu.cpp`

```cpp
#include "GridMenu.h"
#include <HalDisplay.h>

void GridMenu::initializeGrid() {
  // 6 menu items: My Library, Recent Books, Settings, File Transfer, Features, Bluetooth
  items = {
    {"My Library", 0, 0, itemWidth, itemHeight},
    {"Recent", 0, 0, itemWidth, itemHeight},
    {"Settings", 0, 0, itemWidth, itemHeight},
    {"File Transfer", 0, 0, itemWidth, itemHeight},
    {"Features", 0, 0, itemWidth, itemHeight},
    {"Bluetooth", 0, 0, itemWidth, itemHeight},
  };
  calculatePositions();
}

void GridMenu::calculatePositions() {
  int startX = 10;
  for (int i = 0; i < items.size(); i++) {
    int row = i / cols;
    int col = i % cols;
    items[i].x = startX + col * (itemWidth + gap);
    items[i].y = startY + row * (itemHeight + gap);
  }
}

void GridMenu::render() {
  for (int i = 0; i < items.size(); i++) {
    const MenuItem& item = items[i];
    EpdColor bgColor = (i == selectedIndex) ? EpdColor::BLACK : EpdColor::WHITE;
    EpdColor textColor = (i == selectedIndex) ? EpdColor::WHITE : EpdColor::BLACK;

    // Draw button background
    renderer.drawFilledRect(item.x, item.y, item.width, item.height, bgColor);
    renderer.drawRect(item.x, item.y, item.width, item.height, EpdColor::BLACK);

    // Draw label
    renderer.drawString(item.x + 5, item.y + 20, item.label.c_str(),
                       EpdFontFamily::NORMAL, 10, textColor);
  }
}

void GridMenu::selectNext() {
  selectedIndex = (selectedIndex + 1) % items.size();
}

void GridMenu::selectPrevious() {
  selectedIndex = (selectedIndex - 1 + items.size()) % items.size();
}

void GridMenu::selectUp() {
  int row = selectedIndex / cols;
  if (row > 0) {
    selectedIndex -= cols;
  }
}

void GridMenu::selectDown() {
  int row = selectedIndex / cols;
  if (row < rows - 1) {
    selectedIndex += cols;
  }
}
```

---

## 🕐 Bước 3: Tạo TimeManager & WeatherManager

File: `src/util/TimeManager.h`

```cpp
#pragma once
#include <ctime>
#include <string>

class TimeManager {
 private:
  time_t lastSyncTime = 0;
  bool hasWiFi = false;

 public:
  static TimeManager& getInstance();
  
  std::string getCurrentTimeString();
  std::string getCurrentDateString();
  
  // Offline mode: set time manually
  void setManualTime(int hour, int minute, int second);
  
  // Online mode: sync from NTP server
  void syncTimeFromNTP();
  
  void setWiFiConnected(bool connected) { hasWiFi = connected; }
  bool isWiFiConnected() const { return hasWiFi; }
};
```

File: `src/util/WeatherManager.h`

```cpp
#pragma once
#include <string>
#include <ArduinoJson.h>

class WeatherManager {
 private:
  float currentTemp = 0;
  std::string weatherDescription = "N/A";
  time_t lastUpdateTime = 0;
  bool hasWiFi = false;

 public:
  static WeatherManager& getInstance();
  
  std::string getTemperatureString();
  std::string getWeatherDescription();
  
  // Fetch from OpenWeatherMap API (requires WiFi)
  void updateWeather(const std::string& city = "Default");
  
  void setWiFiConnected(bool connected) { hasWiFi = connected; }
  bool isWiFiConnected() const { return hasWiFi; }
  
 private:
  void parseWeatherJSON(const JsonDocument& doc);
};
```

---

## 🎨 Bước 4: Tạo NewGridHomeActivity

File: `src/activities/home/NewGridHomeActivity.h`

```cpp
#pragma once
#include "../Activity.h"
#include "../../components/StatusBar.h"
#include "../../components/GridMenu.h"
#include <functional>
#include <vector>

class NewGridHomeActivity final : public Activity {
 private:
  StatusBar statusBar;
  GridMenu gridMenu;
  
  // Callbacks
  const std::function<void(const std::string&)> onSelectBook;
  const std::function<void()> onMyLibraryOpen;
  const std::function<void()> onRecentsOpen;
  const std::function<void()> onSettingsOpen;
  const std::function<void()> onFileTransferOpen;
  const std::function<void()> onFeaturesOpen;
  const std::function<void()> onBluetoothOpen;

 public:
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
        onSelectBook(onSelectBook),
        onMyLibraryOpen(onMyLibraryOpen),
        onRecentsOpen(onRecentsOpen),
        onSettingsOpen(onSettingsOpen),
        onFileTransferOpen(onFileTransferOpen),
        onFeaturesOpen(onFeaturesOpen),
        onBluetoothOpen(onBluetoothOpen) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(Activity::RenderLock&&) override;

 private:
  void handleMenuSelection();
};
```

File: `src/activities/home/NewGridHomeActivity.cpp`

```cpp
#include "NewGridHomeActivity.h"

void NewGridHomeActivity::onEnter() {
  LOG_DBG("GRID_HOME", "Entering NewGridHomeActivity");
}

void NewGridHomeActivity::onExit() {
  LOG_DBG("GRID_HOME", "Exiting NewGridHomeActivity");
}

void NewGridHomeActivity::loop() {
  // Handle input
  if (mappedInputManager.wasPressed(MappedInputManager::Button::Right)) {
    gridMenu.selectNext();
  }
  if (mappedInputManager.wasPressed(MappedInputManager::Button::Left)) {
    gridMenu.selectPrevious();
  }
  if (mappedInputManager.wasPressed(MappedInputManager::Button::Up)) {
    gridMenu.selectUp();
  }
  if (mappedInputManager.wasPressed(MappedInputManager::Button::Down)) {
    gridMenu.selectDown();
  }
  if (mappedInputManager.wasPressed(MappedInputManager::Button::Ok)) {
    handleMenuSelection();
  }

  // Request render
  if (needsRender()) {
    requestRender();
  }
}

void NewGridHomeActivity::render(Activity::RenderLock&& lock) {
  renderer.clearBuffer(EpdColor::WHITE);

  // Render status bar (time, temp, battery)
  statusBar.render();

  // Render recent books
  // TODO: Add recent books rendering (similar to current HomeActivity)

  // Render grid menu
  gridMenu.render();

  // Update display
  renderer.display();
}

void NewGridHomeActivity::handleMenuSelection() {
  int selectedIndex = gridMenu.getSelectedIndex();
  
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
      break;
  }
}
```

---

## ⚡ Bước 5: Tích Hợp Vào main.cpp

Sửa trong `src/main.cpp`:

```cpp
// ...existing includes...
#include "activities/home/NewGridHomeActivity.h"

// ...existing code...

void onGoHome() {
  exitActivity();
  enterNewActivity(new NewGridHomeActivity(
      renderer, mappedInputManager, onGoToReader, onGoToMyLibrary,
      onGoToRecentBooks, onGoToSettings, onGoToFileTransfer,
      onFeaturesOpen, onBluetoothOpen));  // ← Thêm 2 callback mới
}

// Thêm các callback mới:
void onFeaturesOpen() {
  exitActivity();
  // TODO: Tạo FeatureActivity (Time/Weather Settings)
  enterNewActivity(new SettingsActivity(renderer, mappedInputManager, onGoHome));
}

void onBluetoothOpen() {
  exitActivity();
  // TODO: Tạo BluetoothActivity (Bluetooth pairing)
  enterNewActivity(new SettingsActivity(renderer, mappedInputManager, onGoHome));
}
```

---

## 📱 Bước 6: Tạo Features Activity (Time/Weather Settings)

File: `src/activities/settings/FeaturesActivity.h`

```cpp
#pragma once
#include "../Activity.h"
#include <functional>

class FeaturesActivity final : public Activity {
 private:
  int selectedFeature = 0;  // 0: Time, 1: Weather, 2: Bluetooth
  bool hasWiFi = false;

 public:
  explicit FeaturesActivity(GfxRenderer& renderer,
                           MappedInputManager& mappedInput,
                           const std::function<void()>& onGoBack)
      : Activity("Features", renderer, mappedInput) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(Activity::RenderLock&&) override;

 private:
  void renderTimeSettings();
  void renderWeatherSettings();
};
```

---

## 🔧 Bước 7: Build & Test

```bash
# Build
pio run

# Upload
pio run --target upload

# Monitor
pio device monitor
```

---

## ✅ Checklist Implementation

- [ ] Tạo StatusBar component (time, temp, battery)
- [ ] Tạo GridMenu component (3x2 grid buttons)
- [ ] Tạo TimeManager (offline + NTP sync)
- [ ] Tạo WeatherManager (API integration)
- [ ] Tạo NewGridHomeActivity
- [ ] Tạo FeaturesActivity (Time/Weather settings)
- [ ] Tạo BluetoothActivity (BLE pairing)
- [ ] Integrate vào main.cpp
- [ ] Test trên board
- [ ] Fine-tune UI/layout

---

## 💡 Ghi Chú

1. **Time Management:**
   - Offline: Cho phép người dùng set thủ công ngày/giờ
   - Online: Sync từ NTP server khi có WiFi

2. **Weather:**
   - Offline: Hiển thị "N/A"
   - Online: Fetch từ OpenWeatherMap API

3. **Bluetooth:**
   - Scan available devices
   - Pair & connect
   - Store credentials

4. **UI Responsive:**
   - Navigation: Up/Down/Left/Right
   - Select: OK button
   - Back: Back button

---

Bạn muốn tôi tạo code chi tiết cho từng component không? 🚀
