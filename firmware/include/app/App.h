#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "ipsb/playlist.h"
#include "types.h"

namespace ipsb {

class StorageManager;
class BatteryMonitor;
class AudioEngine;
class UiRenderer;

class App {
  public:
    App(StorageManager& storage, BatteryMonitor& battery, AudioEngine& audio, UiRenderer& ui);

    void begin();
    void run();

  private:
    void handleInput(const InputEvent& event);
    void handleTelemetry();
    void handleMenuSelect();
    void refreshTracks();
    void playIndex(size_t index);
    void sendCommand(AudioCmdType type, uint32_t value = 0, const char* path = nullptr);
    void updateBattery();
    void setStatus(const char* text);
    void clampBrowseTop();
    void copyText(char* dst, size_t dstSize, const char* src);

    StorageManager& storage_;
    BatteryMonitor& battery_;
    AudioEngine& audio_;
    UiRenderer& ui_;

    AppState state_{};
    std::vector<TrackEntry> tracks_;
    PlaylistCursor cursor_;

    uint32_t lastBatteryMs_ = 0;
    uint32_t lastProgressMs_ = 0;
    bool dirty_ = true;
    bool full_ = true;
};

}  // namespace ipsb
