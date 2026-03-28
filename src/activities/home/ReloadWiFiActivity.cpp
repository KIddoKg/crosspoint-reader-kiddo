#include "ReloadWiFiActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>
#include <WiFi.h>

#include <map>

#include "MappedInputManager.h"
#include "../network/WifiCredentialStore.h"
#include "../util/KeyboardEntryActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "util/TimeManager.h"
#include "util/WeatherManager.h"

void ReloadWiFiActivity::onEnter() {
  Activity::onEnter();

  {
    RenderLock lock(*this);
    WIFI_STORE.loadFromFile();
  }

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

  uint8_t mac[6];
  WiFi.macAddress(mac);
  char macStr[64];
  snprintf(macStr, sizeof(macStr), "%s %02x-%02x-%02x-%02x-%02x-%02x", tr(STR_MAC_ADDRESS), mac[0], mac[1], mac[2],
           mac[3], mac[4], mac[5]);
  cachedMacAddress = std::string(macStr);

  requestUpdate();

  const std::string lastSsid = WIFI_STORE.getLastConnectedSsid();
  if (!lastSsid.empty()) {
    const auto* cred = WIFI_STORE.findCredential(lastSsid);
    if (cred) {
      selectedSSID = cred->ssid;
      enteredPassword = cred->password;
      selectedRequiresPassword = !cred->password.empty();
      usedSavedPassword = true;
      attemptConnection();
      return;
    }
  }

  startWifiScan();
}

void ReloadWiFiActivity::onExit() {
  Activity::onExit();
  WiFi.scanDelete();
}

void ReloadWiFiActivity::startWifiScan() {
  state = WifiSelectionState::SCANNING;
  networks.clear();
  requestUpdate();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.scanNetworks(true);
}

void ReloadWiFiActivity::processWifiScanResults() {
  const int16_t scanResult = WiFi.scanComplete();
  if (scanResult == WIFI_SCAN_RUNNING) return;
  if (scanResult == WIFI_SCAN_FAILED) {
    state = WifiSelectionState::NETWORK_LIST;
    requestUpdate();
    return;
  }

  std::map<std::string, WifiNetworkInfo> uniqueNetworks;
  for (int i = 0; i < scanResult; i++) {
    std::string ssid = WiFi.SSID(i).c_str();
    if (ssid.empty()) continue;
    const int32_t rssi = WiFi.RSSI(i);
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
  for (const auto& pair : uniqueNetworks) networks.push_back(pair.second);
  std::sort(networks.begin(), networks.end(), [](const WifiNetworkInfo& a, const WifiNetworkInfo& b) {
    if (a.hasSavedPassword != b.hasSavedPassword) return a.hasSavedPassword;
    return a.rssi > b.rssi;
  });

  WiFi.scanDelete();
  state = WifiSelectionState::NETWORK_LIST;
  selectedNetworkIndex = 0;
  requestUpdate();
}

void ReloadWiFiActivity::selectNetwork(const int index) {
  if (index < 0 || index >= static_cast<int>(networks.size())) return;
  const auto& network = networks[index];
  selectedSSID = network.ssid;
  selectedRequiresPassword = network.isEncrypted;
  usedSavedPassword = false;
  enteredPassword.clear();

  const auto* savedCred = WIFI_STORE.findCredential(selectedSSID);
  if (savedCred && !savedCred->password.empty()) {
    enteredPassword = savedCred->password;
    usedSavedPassword = true;
    attemptConnection();
    return;
  }

  if (selectedRequiresPassword) {
    state = WifiSelectionState::PASSWORD_ENTRY;
    enterNewActivity(new KeyboardEntryActivity(renderer, mappedInput, tr(STR_ENTER_WIFI_PASSWORD), "", 64, false,
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
    xTaskCreate([](void*) {
        vTaskDelay(pdMS_TO_TICKS(3000));
        WeatherManager::getInstance().setWiFiConnected(true);
        vTaskDelete(nullptr);
    }, "ReloadSync", 8192, nullptr, 1, nullptr);
}

void ReloadWiFiActivity::checkConnectionStatus() {
  if (state != WifiSelectionState::CONNECTING) return;
  const wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
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

  if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL || (millis() - connectionStartTime > CONNECTION_TIMEOUT_MS)) {
    connectionError = tr(status == WL_NO_SSID_AVAIL ? STR_ERROR_NETWORK_NOT_FOUND : STR_ERROR_GENERAL_FAILURE);
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
    if (mappedInput.wasPressed(MappedInputManager::Button::Up) || mappedInput.wasPressed(MappedInputManager::Button::Left)) {
      if (savePromptSelection > 0) { savePromptSelection--; requestUpdate(); }
    } else if (mappedInput.wasPressed(MappedInputManager::Button::Down) || mappedInput.wasPressed(MappedInputManager::Button::Right)) {
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

  if (state == WifiSelectionState::CONNECTED || state == WifiSelectionState::CONNECTION_FAILED) {
    if (mappedInput.wasPressed(MappedInputManager::Button::Confirm) || mappedInput.wasPressed(MappedInputManager::Button::Back)) {
      onComplete();
    }
    return;
  }

  if (state == WifiSelectionState::NETWORK_LIST) {
    if (mappedInput.wasPressed(MappedInputManager::Button::Back)) { onComplete(); return; }
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
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_WIFI_NETWORKS), countStr);
  GUI.drawSubHeader(renderer, Rect{0, metrics.topPadding + metrics.headerHeight, pageWidth, metrics.tabBarHeight}, cachedMacAddress.c_str());

  switch (state) {
    case WifiSelectionState::SCANNING: renderConnecting(); break;
    case WifiSelectionState::NETWORK_LIST: renderNetworkList(); break;
    case WifiSelectionState::CONNECTING: renderConnecting(); break;
    case WifiSelectionState::CONNECTED: renderConnected(); break;
    case WifiSelectionState::SAVE_PROMPT: renderSavePrompt(); break;
    case WifiSelectionState::CONNECTION_FAILED: renderConnectionFailed(); break;
    default: break;
  }
  renderer.displayBuffer();
}

void ReloadWiFiActivity::renderNetworkList() const {
  auto metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  if (networks.empty()) {
    renderer.drawCenteredText(UI_10_FONT_ID, pageHeight/2, tr(STR_NO_NETWORKS));
  } else {
    int contentTop = metrics.topPadding + metrics.headerHeight + metrics.tabBarHeight + metrics.verticalSpacing;
    int contentHeight = pageHeight - contentTop - metrics.buttonHintsHeight - metrics.verticalSpacing * 2;
    GUI.drawList(renderer, Rect{0, contentTop, pageWidth, contentHeight}, static_cast<int>(networks.size()),
        selectedNetworkIndex, [this](int index) { return networks[index].ssid; }, nullptr, nullptr,
        [this](int index) {
          auto network = networks[index];
          return std::string(network.hasSavedPassword ? "+ " : "") + (network.isEncrypted ? "* " : "") + getSignalStrengthIndicator(network.rssi);
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
  renderer.drawCenteredText(UI_12_FONT_ID, centerY - 20, tr(STR_CONNECTED), true, EpdFontFamily::BOLD);
  renderer.drawCenteredText(UI_10_FONT_ID, centerY + 10, "DateTime & Weather Syncing...");
  const auto labels = mappedInput.mapLabels("", tr(STR_DONE), "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
}

void ReloadWiFiActivity::renderSavePrompt() const {
  const auto pageWidth = renderer.getScreenWidth();
  const auto centerY = renderer.getScreenHeight() / 2;
  renderer.drawCenteredText(UI_10_FONT_ID, centerY, tr(STR_SAVE_PASSWORD));
  const int buttonY = centerY + 40;
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
  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_DONE), "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
}
