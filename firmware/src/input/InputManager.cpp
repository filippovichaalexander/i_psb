#include "input/InputManager.h"

#include <Arduino.h>

#include "system/Logger.h"
#include "system/Runtime.h"

namespace ipsb {

void InputManager::begin() {
    const ButtonState initial[5] = {
        {pins::kBtnMenu, ButtonId::Menu, false, false, 0, 0, false},
        {pins::kBtnPlay, ButtonId::Play, false, false, 0, 0, false},
        {pins::kBtnPrev, ButtonId::Prev, false, false, 0, 0, false},
        {pins::kBtnNext, ButtonId::Next, false, false, 0, 0, false},
        {pins::kBtnSelect, ButtonId::Select, false, false, 0, 0, false},
    };
    for (size_t i = 0; i < 5; ++i) {
        buttons_[i] = initial[i];
        pinMode(buttons_[i].pin, INPUT_PULLUP);
        const bool pressed = digitalRead(buttons_[i].pin) == LOW;
        buttons_[i].raw = pressed;
        buttons_[i].stable = pressed;
        buttons_[i].changedMs = millis();
    }

    pinMode(pins::kTtpScl, OUTPUT);
    digitalWrite(pins::kTtpScl, HIGH);
    pinMode(pins::kTtpSdo, INPUT_PULLUP);

    pinMode(pins::kVibro, OUTPUT);
    digitalWrite(pins::kVibro, LOW);

    Logger::info("input", "buttons + TTP229 ready (SCL=%d SDO=%d)", pins::kTtpScl, pins::kTtpSdo);
}

void InputManager::task() {
    for (;;) {
        poll(millis());
        vTaskDelay(pdMS_TO_TICKS(kInputPollMs));
    }
}

void InputManager::poll(uint32_t nowMs) {
    pollButtons(nowMs);
    pollWheel(nowMs);

    if (vibroUntilMs_ != 0 && static_cast<int32_t>(nowMs - vibroUntilMs_) >= 0) {
        setVibro(false);
        vibroUntilMs_ = 0;
    }
}

void InputManager::pollButtons(uint32_t nowMs) {
    for (size_t i = 0; i < 5; ++i) {
        ButtonState& button = buttons_[i];
        const bool pressed = digitalRead(button.pin) == LOW;

        if (pressed != button.raw) {
            button.raw = pressed;
            button.changedMs = nowMs;
        }

        if ((nowMs - button.changedMs) < kButtonDebounceMs) {
            continue;
        }

        if (button.raw != button.stable) {
            button.stable = button.raw;
            if (button.stable) {
                button.pressedMs = nowMs;
                button.longSent = false;
            } else if (!button.longSent) {
                emitButton(button.id, PressType::Short);
            }
        }

        if (button.stable && !button.longSent && (nowMs - button.pressedMs) >= kButtonLongPressMs) {
            button.longSent = true;
            emitButton(button.id, PressType::Long);
        }
    }
}

void InputManager::pollWheel(uint32_t nowMs) {
    const uint8_t mask = readTtpMask();
    lastMask_ = mask;

    const float delta = wheel_.feed(mask, nowMs);
    if (delta == 0.0f) {
        return;
    }

    wheelAccum_ += delta;
    bool ticked = false;
    while (wheelAccum_ >= 1.0f) {
        emitWheel(1);
        wheelAccum_ -= 1.0f;
        ticked = true;
    }
    while (wheelAccum_ <= -1.0f) {
        emitWheel(-1);
        wheelAccum_ += 1.0f;
        ticked = true;
    }

    if (ticked) {
        setVibro(true);
        vibroUntilMs_ = millis() + kHapticPulseMs;
    }
}

uint8_t InputManager::readTtpMask() {
    uint16_t data = 0;
    for (int i = 0; i < 16; ++i) {
        digitalWrite(pins::kTtpScl, LOW);
        delayMicroseconds(2);
        digitalWrite(pins::kTtpScl, HIGH);
        delayMicroseconds(2);
        if (digitalRead(pins::kTtpSdo) == LOW) {
            data |= static_cast<uint16_t>(1u << i);
        }
    }
    // The wheel uses TP0..TP7. TP8..TP15 are unused on the trimmed board.
    return static_cast<uint8_t>(data & 0xFF);
}

void InputManager::emitButton(ButtonId id, PressType press) {
    if (gRuntime.inputQueue == nullptr) {
        return;
    }
    InputEvent event{};
    event.type = InputType::Button;
    event.button = id;
    event.press = press;
    if (xQueueSend(gRuntime.inputQueue, &event, 0) != pdTRUE) {
        Logger::warn("input", "queue full, button dropped");
    }
}

void InputManager::emitWheel(int value) {
    if (gRuntime.inputQueue == nullptr) {
        return;
    }
    InputEvent event{};
    event.type = InputType::Wheel;
    event.value = static_cast<int16_t>(value);
    if (xQueueSend(gRuntime.inputQueue, &event, 0) != pdTRUE) {
        Logger::warn("input", "queue full, wheel dropped");
    }
}

void InputManager::setVibro(bool on) {
    digitalWrite(pins::kVibro, on ? HIGH : LOW);
}

}  // namespace ipsb
