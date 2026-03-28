#include "ReloadWiFiActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>
#include <WiFi.h>

#include <map>
#include <algorithm>

#include "MappedInputManager.h"
#include "../../WifiCredentialStore.h"
#include "../util/KeyboardEntryActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "util/TimeManager.h"
#include "util/WeatherManager.h"

void ReloadWiFiActivity::onEnter() {
  Activity::onEnter();

  // Load saved WiFi credentials
  {
    RenderLock lock(*this);
    WIFI_STORE.loadFromFile();
  }

  // Reset state
  selectedNetworkIndex = 0;
  networks.clear();
  state = WifiSelectionState::SCANNING;
  selectedSSID.clear();
  connectedIP.clear();
  connectionError.clear();
  enteredPassword.clear();
  usedSavedPassword = false;
  savePromptSelection = 0;
  forgetPromptSelection = 0;

  // Cache MAC address for display
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char macStr[64];
  snprintf(macStr, sizeof(macStr), "%s %02x-%02x-%02x-%02x-%02x-%02x", tr(STR_MAC_ADDRESS), mac[0], mac[1], mac[2],
           mac[3], mac[4], mac[5]);
  cachedMacAddress = std::string(macStr);

  // Trigger first update to show scanning message
  requestUpdate();

  // Attempt to auto-connect to the last network
  const std::string lastSsid = WIFI_STORE.getLastConnectedSsid();
  if (!lastSsid.empty()) {
    const auto* cred = WIFI_STORE.findCredential(lastSsid);
    if (cred) {
      LOG_DBG("WIFI", "Attempting to auto-connect to %s", lastSsid.c_str());
      selectedSSID = cred->ssid;
      enteredPassword = cred->password;
      selectedRequiresPassword = !cred->password.empty();
      usedSavedPassword = true;
      attemptConnection();
      requestUpdate();
      return;
    }
  }

  // Fallback to scanning
  startWifiScan();
}

void ReloadWiFiActivity::onExit() {
  ActivityWithSubactivity::onExit();
  WiFi.scanDelete();
}

void ReloadWiFiActivity::startWifiScan() {
  state = WifiSelectionState::SCANNING;
  networks.clear();
  requestUpdate();

  // Set WiFi mode to station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Start async scan
  WiFi.scanNetworks(true);  // true = async scan
}

void ReloadWiFiActivity::processWifiScanResults() {
  const int16_t scanResult = WiFi.scanComplete();

  if (scanResult == WIFI_SCAN_RUNNING) {
    return;
  }

  if (scanResult == WIFI_SCAN_FAILED) {
    state = WifiSelectionState::NETWORK_LIST;
    requestUpdate();
    return;
  }

  std::map<std::string, WifiNetworkInfo> uniqueNetworks;

  for (int i = 0; i < scanResult; i++) {
    std::string ssid = WiFi.SSID(i).c_str();
    const int32_t rssi = WiFi.RSSI(i);

    if (ssid.empty()) {
      continue;
    }

    auto it = uniqueNetworks.find(ssid);
    if (it == uniqueNetworks.end() || rssi > it->second.rssi) {
      WifiNetworkInfo network;
      network.ssid = ssid;
      network.rssi = rssi;
      network.isEncrypted = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
      network.hasSavedPassword = WIFI_STORE.hasSavedCredential(network.ssid);
      uniqueNetworks[ssid] = network;
    }
  }

  networks.clear();
  for (const auto& pair : uniqueNetworks) {
    networks.push_back(pair.second);
  }

  std::sort(networks.begin(), networks.end(), [](const WifiNetworkInfo& a, const WifiNetworkInfo& b) {
    if (a.hasSavedPassword != b.hasSavedPassword) {
      return a.hasSavedPassword;
    }
    return a.rssi > b.rssi;
  });

  WiFi.scanDelete();
  state = WifiSelectionState::NETWORK_LIST;
  selectedNetworkIndex = 0;
  requestUpdate();
}

void ReloadWiFiActivity::selectNetwork(const int index) {
  if (index < 0 || index >= static_cast<int>(networks.size())) {
    return;
  }

  const auto& network = networks[index];
  selectedSSID = network.ssid;
  selectedRequiresPassword = network.isEncrypted;
  usedSavedPassword = false;
  enteredPassword.clear();

  const auto* savedCred = WIFI_STORE.findCredential(selectedSSID);
  if (savedCred && !savedCred->password.empty()) {
    enteredPassword = savedCred->password;
    usedSavedPassword = true;
    LOG_DBG("WiFi", "Using saved password for %s", selectedSSID.c_str());
    attemptConnection();
    return;
  }

  if (selectedRequiresPassword) {
    state = WifiSelectionState::PASSWORD_ENTRY;
    enterNewActivity(new KeyboardEntryActivity(
        renderer, mappedInput, tr(STR_ENTER_WIFI_PASSWORD),
        "", 64, false,
        [this](const std::string& text) {
          enteredPassword = text;
          exitActivity();
        },
        [this] {
          state = WifiSelectionState::NETWORK_LIST;
          exitActivity();
          requestUpdate();
        }));
  } else {
    attemptConnection();
  }
}

void ReloadWiFiActivity::attemptConnection() {
  state = WifiSelectionState::CONNECTING;
  connectionStartTime = millis();
  connectedIP.clear();
  connectionError.clear();
  requestUpdate();

  WiFi.mode(WIFI_STA);

  if (selectedRequiresPassword && !enteredPassword.empty()) {
    WiFi.begin(selectedSSID.c_str(), enteredPassword.c_str());
  } else {
    WiFi.begin(selectedSSID.c_str());
  }
}

void ReloadWiFiActivity::syncDateTimeAndWeather() {
    TimeManager::getInstance().setWiFiConnected(true);
    xTaskCreate(
    [](void*) {
        vTaskDelay(pdMS_TO_TICKS(3000));
        WeatherManager::getInstance().setWiFiConnected(true);
        vTaskDelete(nullptr);
    },
    "ReloadSync",
    8192,
    nullptr,
    1,
    nullptr);
}

void ReloadWiFiActivity::checkConnectionStatus() {
  if (state != WifiSelectionState::CONNECTING) {
    return;
  }

  wl_status_t wifiStatus = WiFi.status();

  if (wifiStatus == WL_CONNECTED) {
    IPAddress ip = WiFi.localIP();
    connectedIP = ip.toString().c_str();

    {
      RenderLock lock(*this);
      WIFI_STORE.setLastConnectedSsid(selectedSSID);
    }

    syncDateTimeAndWeather();

    if (!usedSavedPassword && !enteredPassword.empty()) {
      state = WifiSelectionState::SAVE_PROMPT;
      savePromptSelection = 0;
      requestUpdate();
    } else {
      state = WifiSelectionState::CONNECTED;
      requestUpdate();
    }
    return;
  }

  if (wifiStatus == WL_CONNECT_FAILED || wifiStatus == WL_NO_SSID_AVAIL || (millis() - connectionStartTime > CONNECTION_TIMEOUT_MS)) {
    connectionError = (wifiStatus == WL_NO_SSID_AVAIL)
        ? tr(STR_ERROR_NETWORK_NOT_FOUND)
        : tr(STR_ERROR_GENERAL_FAILURE);
    if (millis() - connectionStartTime > CONNECTION_TIMEOUT_MS) {
        WiFi.disconnect();
        connectionError = tr(STR_ERROR_CONNECTION_TIMEOUT);
    }
    state = WifiSelectionState::CONNECTION_FAILED;
    requestUpdate();
  }
}

void ReloadWiFiActivity::loop() {
  if (subActivity) {
    subActivity->loop();
    return;
  }

  if (state == WifiSelectionState::SCANNING) {
    processWifiScanResults();
    return;
  }

  if (state == WifiSelectionState::CONNECTING) {
    checkConnectionStatus();
    return;
  }

  if (state == WifiSelectionState::PASSWORD_ENTRY) {
    attemptConnection();
    return;
  }

  if (state == WifiSelectionState::SAVE_PROMPT) {
    if (mappedInput.wasPressed(MappedInputManager::Button::Up) ||
        mappedInput.wasPressed(MappedInputManager::Button::Left)) {
      if (savePromptSelection > 0) { savePromptSelection--; requestUpdate(); }
    } else if (mappedInput.wasPressed(MappedInputManager::Button::Down) ||
               mappedInput.wasPressed(MappedInputManager::Button::Right)) {
      if (savePromptSelection < 1) { savePromptSelection++; requestUpdate(); }
    } else if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
      if (savePromptSelection == 0) {
        RenderLock lock(*this);
        WIFI_STORE.addCredential(selectedSSID, enteredPassword);
      }
      state = WifiSelectionState::CONNECTED;
      requestUpdate();
    } else if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
      state = WifiSelectionState::CONNECTED;
      requestUpdate();
    }
    return;
  }

  if (state == WifiSelectionState::FORGET_PROMPT) {
    if (mappedInput.wasPressed(MappedInputManager::Button::Up) ||
        mappedInput.wasPressed(MappedInputManager::Button::Left)) {
      if (forgetPromptSelection > 0) { forgetPromptSelection--; requestUpdate(); }
    } else if (mappedInput.wasPressed(MappedInputManager::Button::Down) ||
               mappedInput.wasPressed(MappedInputManager::Button::Right)) {
      if (forgetPromptSelection < 1) { forgetPromptSelection++; requestUpdate(); }
    } else if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
      if (forgetPromptSelection == 1) {
        RenderLock lock(*this);
        WIFI_STORE.removeCredential(selectedSSID);
        auto it = std::find_if(networks.begin(), networks.end(), [this](const WifiNetworkInfo& n) { return n.ssid == selectedSSID; });
        if (it != networks.end()) it->hasSavedPassword = false;
      }
      startWifiScan();
    } else if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
      startWifiScan();
    }
    return;
  }

  if (state == WifiSelectionState::CONNECTED || state == WifiSelectionState::CONNECTION_FAILED) {
    if (mappedInput.wasPressed(MappedInputManager::Button::Back) ||
        mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
      onComplete();
      return;
    }
  }

  if (state == WifiSelectionState::NETWORK_LIST) {
    if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
      onComplete();
      return;
    }

    if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
      if (!networks.empty()) selectNetwork(selectedNetworkIndex);
      else startWifiScan();
      return;
    }

    buttonNavigator.onNext([this] {
      selectedNetworkIndex = ButtonNavigator::nextIndex(selectedNetworkIndex, networks.size());
      requestUpdate();
    });

    buttonNavigator.onPrevious([this] {
      selectedNetworkIndex = ButtonNavigator::previousIndex(selectedNetworkIndex, networks.size());
      requestUpdate();
    });
  }
}

std::string ReloadWiFiActivity::getSignalStrengthIndicator(const int32_t rssi) const {
  if (rssi >= -50) return "||||";
  if (rssi >= -60) return " |||";
  if (rssi >= -70) return "  ||";
  return "   |";
}

void ReloadWiFiActivity::render(Activity::RenderLock&& lock) {
  if (state == WifiSelectionState::PASSWORD_ENTRY) {
    requestUpdateAndWait();
    return;
  }

  renderer.clearScreen();

  auto metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  char countStr[32];
  snprintf(countStr, sizeof(countStr), tr(STR_NETWORKS_FOUND), networks.size());
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_WIFI_NETWORKS),
                 countStr);
  GUI.drawSubHeader(renderer, Rect{0, metrics.topPadding + metrics.headerHeight, pageWidth, metrics.tabBarHeight},
                    cachedMacAddress.c_str());

  switch (state) {
    case WifiSelectionState::SCANNING: renderConnecting(); break;
    case WifiSelectionState::NETWORK_LIST: renderNetworkList(); break;
    case WifiSelectionState::CONNECTING: renderConnecting(); break;
    case WifiSelectionState::CONNECTED: renderConnected(); break;
    case WifiSelectionState::SAVE_PROMPT: renderSavePrompt(); break;
    case WifiSelectionState::CONNECTION_FAILED: renderConnectionFailed(); break;
    case WifiSelectionState::FORGET_PROMPT: renderForgetPrompt(); break;
  }

  renderer.displayBuffer();
}

void ReloadWiFiActivity::renderNetworkList() const {
  auto metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  if (networks.empty()) {
    renderer.drawCenteredText(UI_10_FONT_ID, pageHeight / 2, tr(STR_NO_NETWORKS));
  } else {
    int contentTop = metrics.topPadding + metrics.headerHeight + metrics.tabBarHeight + metrics.verticalSpacing;
    int contentHeight = pageHeight - contentTop - metrics.buttonHintsHeight - metrics.verticalSpacing * 2;
    GUI.drawList(
        renderer, Rect{0, contentTop, pageWidth, contentHeight}, static_cast<int>(networks.size()),
        selectedNetworkIndex, [this](int index) { return networks[index].ssid; }, nullptr, nullptr,
        [this](int index) {
          auto network = networks[index];
          return std::string(network.hasSavedPassword ? "+ " : "") + (network.isEncrypted ? "* " : "") +
                 getSignalStrengthIndicator(network.rssi);
        });
  }

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_CONNECT), "", tr(STR_RETRY));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
}

void ReloadWiFiActivity::renderConnecting() const {
  const auto centerY = renderer.getScreenHeight() / 2;
  renderer.drawCenteredText(UI_10_FONT_ID, centerY, state == WifiSelectionState::SCANNING ? tr(STR_SCANNING) : tr(STR_CONNECTING));
}

void ReloadWiFiActivity::renderConnected() const {
  const auto centerY = renderer.getScreenHeight() / 2;
  renderer.drawCenteredText(UI_12_FONT_ID, centerY - 30, tr(STR_CONNECTED), true, EpdFontFamily::BOLD);
  renderer.drawCenteredText(UI_10_FONT_ID, centerY, "DateTime & Weather Syncing...");
  const std::string ipInfo = std::string(tr(STR_IP_ADDRESS_PREFIX)) + connectedIP;
  renderer.drawCenteredText(UI_10_FONT_ID, centerY + 30, ipInfo.c_str());

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), "", "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
}

void ReloadWiFiActivity::renderSavePrompt() const {
  const auto pageWidth = renderer.getScreenWidth();
  const auto centerY = renderer.getScreenHeight() / 2;
  renderer.drawCenteredText(UI_10_FONT_ID, centerY - 20, tr(STR_SAVE_PASSWORD));
  const int buttonY = centerY + 20;
  if (savePromptSelection == 0) renderer.drawText(UI_10_FONT_ID, pageWidth/2 - 40, buttonY, ("[" + std::string(tr(STR_YES)) + "]").c_str());
  else renderer.drawText(UI_10_FONT_ID, pageWidth/2 - 36, buttonY, tr(STR_YES));
  if (savePromptSelection == 1) renderer.drawText(UI_10_FONT_ID, pageWidth/2 + 20, buttonY, ("[" + std::string(tr(STR_NO)) + "]").c_str());
  else renderer.drawText(UI_10_FONT_ID, pageWidth/2 + 24, buttonY, tr(STR_NO));
  const auto labels = mappedInput.mapLabels(tr(STR_CANCEL), tr(STR_SELECT), tr(STR_DIR_LEFT), tr(STR_DIR_RIGHT));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
}

void ReloadWiFiActivity::renderConnectionFailed() const {
  const auto centerY = renderer.getScreenHeight() / 2;
  renderer.drawCenteredText(UI_12_FONT_ID, centerY - 10, tr(STR_CONNECTION_FAILED), true, EpdFontFamily::BOLD);
  renderer.drawCenteredText(UI_10_FONT_ID, centerY + 20, connectionError.c_str());
  const auto labels = mappedInput.mapLabels(tr(STR_BACK), "", "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
}

void ReloadWiFiActivity::renderForgetPrompt() const {
  const auto pageWidth = renderer.getScreenWidth();
  const auto centerY = renderer.getScreenHeight() / 2;
  renderer.drawCenteredText(UI_10_FONT_ID, centerY - 20, tr(STR_FORGET_AND_REMOVE));
  const int buttonY = centerY + 20;
  if (forgetPromptSelection == 0) renderer.drawText(UI_10_FONT_ID, pageWidth/2 - 60, buttonY, ("[" + std::string(tr(STR_CANCEL)) + "]").c_str());
  else renderer.drawText(UI_10_FONT_ID, pageWidth/2 - 56, buttonY, tr(STR_CANCEL));
  if (forgetPromptSelection == 1) renderer.drawText(UI_10_FONT_ID, pageWidth/2 + 20, buttonY, ("[" + std::string(tr(STR_FORGET_BUTTON)) + "]").c_str());
  else renderer.drawText(UI_10_FONT_ID, pageWidth/2 + 24, buttonY, tr(STR_FORGET_BUTTON));
  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_LEFT), tr(STR_DIR_RIGHT));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
}
