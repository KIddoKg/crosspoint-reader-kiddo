#pragma once

#include <vector>
#include <string>
#include <functional>

#include "../Activity.h"
#include "util/ButtonNavigator.h"

class ReadingStatsActivity final : public Activity {
  ButtonNavigator buttonNavigator;
  const std::function<void()> onGoBack;

 public:
  explicit ReadingStatsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                               const std::function<void()>& onGoBack)
      : Activity("ReadingStats", renderer, mappedInput),
        onGoBack(onGoBack) {}
  
  void onEnter() override;
  void loop() override;
  void render(Activity::RenderLock&&) override;
};
