#pragma once
#include <GfxRenderer.h>
#include <ctime>
#include <string>

class StatusBar {
 private:
  GfxRenderer& renderer;
  int y = 0;  // Y position (top of screen)
  int height = 25;  // Status bar height
  float currentTemp = 0;
  int batteryPercent = 85;
  bool hasWiFi = false;

 public:
  explicit StatusBar(GfxRenderer& renderer) : renderer(renderer) {}

  void render();
  void setTemperature(float temp) { currentTemp = temp; }
  void setBatteryPercent(int percent) { batteryPercent = percent; }
  void setWiFiConnected(bool connected) { hasWiFi = connected; }

 private:
  std::string getTimeString() const;
  std::string getTemperatureString() const;
  void renderBattery(int x, int y, int percent) const;
};
