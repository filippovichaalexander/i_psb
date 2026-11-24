#pragma once

#include <Audio.h>

#include <cstdint>

#include "ipsb/volume.h"
#include "types.h"

namespace ipsb {

class AudioEngine {
  public:
    bool begin();
    void task();
    bool ready() const {
        return ready_;
    }
    void setVolume(uint8_t step);

  private:
    static void onInfo(Audio::msg_t msg);
    void handleCommand(const AudioCommand& command);
    void publish(AudioTelemetryType type);
    void publishError(const char* text);

    Audio audio_;
    bool ready_ = false;
    uint8_t volume_ = kVolumeStartupStep;
    uint32_t lastTelemetryMs_ = 0;
};

}  // namespace ipsb
