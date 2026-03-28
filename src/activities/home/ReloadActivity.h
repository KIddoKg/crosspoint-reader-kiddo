#pragma once

#include <functional>
#include "../ActivityWithSubactivity.h"

class ReloadActivity final : public ActivityWithSubactivity {
  const std::function<void()> onGoBack;

  void onWifiSelectionComplete(bool connected);

 public:
  explicit ReloadActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                         const std::function<void()>& onGoBack)
      : ActivityWithSubactivity("Reload", renderer, mappedInput), onGoBack(onGoBack) {}
  void onEnter() override;
  void loop() override;
  void render(Activity::RenderLock&&) override;
};
