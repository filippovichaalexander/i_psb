#pragma once

#include <cstdint>

#include "config.h"
#include "ipsb/wheel.h"
#include "types.h"

namespace ipsb {

class InputManager {
  public:
    void begin();
    void task();

  private:
    struct ButtonState {
        int pin = -1;
        ButtonId id = ButtonId::Menu;
        bool raw = false;
        bool stable = false;
        uint32_t changedMs = 0;
        uint32_t pressedMs = 0;
        bool longSent = false;
    };

    void poll(uint32_t nowMs);
    void pollButtons(uint32_t nowMs);
    void pollWheel(uint32_t nowMs);
    uint8_t readTtpMask();
    void emitButton(ButtonId id, PressType press);
    void emitWheel(int value);
    void setVibro(bool on);

    ButtonState buttons_[5] = {};
    WheelDecoder wheel_{kSectorCount};
    uint16_t lastMask_ = 0;
    float wheelAccum_ = 0.0f;
    uint32_t vibroUntilMs_ = 0;
};

}  // namespace ipsb
