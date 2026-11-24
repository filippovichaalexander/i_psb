#include "app/App.h"

#include <Arduino.h>

#include <cstdio>
#include <cstring>

#include "audio/AudioEngine.h"
#include "config.h"
#include "power/BatteryMonitor.h"
#include "storage/StorageManager.h"
#include "system/Logger.h"
#include "system/Runtime.h"
#include "ui/UiRenderer.h"

namespace ipsb {
namespace {

constexpr int kMenuItemCount = 6;
constexpr int kSettingsItemCount = 4;
constexpr int kBrowseVisible = 5;

}  // namespace

App::App(StorageManager& storage, BatteryMonitor& battery, AudioEngine& audio, UiRenderer& ui)
    : storage_(storage), battery_(battery), audio_(audio), ui_(ui) {}

void App::copyText(char* dst, size_t dstSize, const char* src) {
    if (dst == nullptr || dstSize == 0) {
        return;
    }
    std::strncpy(dst, src ? src : "", dstSize - 1);
    dst[dstSize - 1] = '\0';
}

void App::setStatus(const char* text) {
    copyText(state_.status, sizeof(state_.status), text);
}

void App::begin() {
    state_ = AppState{};
    state_.volume = kVolumeStartupStep;
    state_.screen = Screen::Menu;
    copyText(state_.player.title, sizeof(state_.player.title), "No track");
    state_.sdReady = storage_.ready();
    state_.audioReady = audio_.ready();
    lastBatteryMs_ = millis();
    lastProgressMs_ = millis();

    refreshTracks();
    ui_.render(state_, tracks_, true);
    Logger::info("app", "ready: tracks=%u sd=%d audio=%d", static_cast<unsigned>(tracks_.size()),
                 state_.sdReady, state_.audioReady);
}

void App::refreshTracks() {
    tracks_.clear();
    if (!storage_.ready()) {
        state_.sdReady = false;
        cursor_.setCount(0);
        setStatus("SD not mounted");
        return;
    }

    storage_.scanMusic(tracks_, kMaxTracks);
    cursor_.setCount(tracks_.size());
    state_.sdReady = true;
    if (tracks_.empty()) {
        setStatus("No music in /Music");
    } else {
        setStatus("");
    }
}

void App::sendCommand(AudioCmdType type, uint32_t value, const char* path) {
    if (gRuntime.audioCmdQueue == nullptr) {
        return;
    }
    AudioCommand command{};
    command.type = type;
    command.value = value;
    if (path != nullptr) {
        copyText(command.path, sizeof(command.path), path);
    }
    if (xQueueSend(gRuntime.audioCmdQueue, &command, 0) != pdTRUE) {
        Logger::warn("app", "audio command queue full");
    }
}

void App::playIndex(size_t index) {
    if (index >= tracks_.size()) {
        return;
    }
    cursor_.setCurrent(index);
    sendCommand(AudioCmdType::Play, 0, tracks_[index].path);

    copyText(state_.player.title, sizeof(state_.player.title), tracks_[index].name);
    state_.player.artist[0] = '\0';
    state_.player.album[0] = '\0';
    state_.player.playing = true;
    state_.player.positionSec = 0;
    state_.player.durationSec = 0;
    state_.screen = Screen::NowPlaying;
    dirty_ = true;
    full_ = true;
}

void App::clampBrowseTop() {
    if (state_.browseIndex < state_.browseTop) {
        state_.browseTop = state_.browseIndex;
    }
    if (state_.browseIndex >= state_.browseTop + kBrowseVisible) {
        state_.browseTop = state_.browseIndex - (kBrowseVisible - 1);
    }
}

void App::handleMenuSelect() {
    switch (state_.menuIndex) {
        case 0:
            refreshTracks();
            state_.browseIndex = 0;
            state_.browseTop = 0;
            state_.screen = Screen::Browse;
            break;
        case 1:
            state_.screen = Screen::NowPlaying;
            break;
        case 2:
            setStatus("Radio is planned for M5");
            state_.screen = Screen::Notice;
            break;
        case 3:
            setStatus("Wi-Fi Sync is planned for M3");
            state_.screen = Screen::Notice;
            break;
        case 4:
            state_.menuIndex = 0;
            state_.screen = Screen::Settings;
            break;
        case 5:
            state_.screen = Screen::About;
            break;
        default:
            break;
    }
    dirty_ = true;
    full_ = true;
}

void App::handleInput(const InputEvent& event) {
    if (event.type == InputType::Wheel) {
        const int delta = event.value;
        switch (state_.screen) {
            case Screen::NowPlaying: {
                int next = static_cast<int>(state_.volume) + delta;
                if (next < 0) {
                    next = 0;
                }
                const uint8_t clamped = clampVolume(static_cast<uint8_t>(next));
                if (clamped != state_.volume) {
                    state_.volume = clamped;
                    sendCommand(AudioCmdType::SetVolume, clamped);
                    dirty_ = true;
                    full_ = false;
                }
                break;
            }
            case Screen::Menu: {
                int next = static_cast<int>(state_.menuIndex) + delta;
                while (next < 0) {
                    next += kMenuItemCount;
                }
                state_.menuIndex = static_cast<uint8_t>(next % kMenuItemCount);
                dirty_ = true;
                full_ = false;
                break;
            }
            case Screen::Browse: {
                if (tracks_.empty()) {
                    break;
                }
                int next = static_cast<int>(state_.browseIndex) + delta;
                const int count = static_cast<int>(tracks_.size());
                while (next < 0) {
                    next += count;
                }
                state_.browseIndex = static_cast<uint8_t>(next % count);
                clampBrowseTop();
                dirty_ = true;
                full_ = false;
                break;
            }
            case Screen::Settings: {
                int next = static_cast<int>(state_.menuIndex) + delta;
                while (next < 0) {
                    next += kSettingsItemCount;
                }
                state_.menuIndex = static_cast<uint8_t>(next % kSettingsItemCount);
                dirty_ = true;
                full_ = false;
                break;
            }
            default:
                break;
        }
        return;
    }

    switch (event.button) {
        case ButtonId::Menu:
            if (state_.screen != Screen::Menu) {
                state_.screen = Screen::Menu;
                dirty_ = true;
                full_ = true;
            }
            break;

        case ButtonId::Play:
            if (state_.screen == Screen::NowPlaying || state_.screen == Screen::Browse) {
                sendCommand(AudioCmdType::PauseResume);
                state_.player.playing = !state_.player.playing;
                dirty_ = true;
                full_ = false;
            }
            break;

        case ButtonId::Prev:
        case ButtonId::Next: {
            const bool forward = event.button == ButtonId::Next;
            if (state_.screen == Screen::NowPlaying && event.press == PressType::Long) {
                int32_t target = static_cast<int32_t>(state_.player.positionSec);
                target += forward ? 10 : -10;
                if (target < 0) {
                    target = 0;
                }
                sendCommand(AudioCmdType::Seek, static_cast<uint32_t>(target));
                dirty_ = true;
                full_ = false;
                break;
            }
            if (tracks_.empty()) {
                break;
            }
            const bool moved = forward ? cursor_.next() : cursor_.previous();
            if (moved) {
                playIndex(cursor_.current());
            }
            break;
        }

        case ButtonId::Select:
            switch (state_.screen) {
                case Screen::Menu:
                    handleMenuSelect();
                    break;
                case Screen::Browse:
                    playIndex(state_.browseIndex);
                    break;
                case Screen::NowPlaying:
                    sendCommand(AudioCmdType::PauseResume);
                    state_.player.playing = !state_.player.playing;
                    dirty_ = true;
                    full_ = false;
                    break;
                case Screen::Settings:
                    if (state_.menuIndex == 0) {
                        if (state_.repeatMode == RepeatMode::Off) {
                            state_.repeatMode = RepeatMode::All;
                        } else if (state_.repeatMode == RepeatMode::All) {
                            state_.repeatMode = RepeatMode::One;
                        } else {
                            state_.repeatMode = RepeatMode::Off;
                        }
                        cursor_.setRepeatMode(state_.repeatMode);
                        dirty_ = true;
                        full_ = false;
                    } else if (state_.menuIndex == 3) {
                        state_.screen = Screen::Menu;
                        dirty_ = true;
                        full_ = true;
                    }
                    break;
                case Screen::About:
                case Screen::Notice:
                case Screen::Error:
                default:
                    state_.screen = Screen::Menu;
                    dirty_ = true;
                    full_ = true;
                    break;
            }
            break;
    }
}

void App::handleTelemetry() {
    if (gRuntime.telemetryQueue == nullptr) {
        return;
    }
    AudioTelemetry telemetry;
    while (xQueueReceive(gRuntime.telemetryQueue, &telemetry, 0) == pdTRUE) {
        switch (telemetry.type) {
            case AudioTelemetryType::Tick:
                state_.player.positionSec = telemetry.positionSec;
                state_.player.durationSec = telemetry.durationSec;
                state_.player.bitrate = telemetry.bitrate;
                state_.player.sampleRate = telemetry.sampleRate;
                state_.player.bits = telemetry.bits;
                if (state_.player.playing != telemetry.running) {
                    state_.player.playing = telemetry.running;
                    dirty_ = true;
                    full_ = false;
                }
                if ((millis() - lastProgressMs_) >= kProgressRefreshMs) {
                    lastProgressMs_ = millis();
                    dirty_ = true;
                    full_ = false;
                }
                break;

            case AudioTelemetryType::TrackEnded:
                if (cursor_.next()) {
                    playIndex(cursor_.current());
                } else {
                    state_.player.playing = false;
                    dirty_ = true;
                    full_ = false;
                }
                break;

            case AudioTelemetryType::TagData: {
                const char* text = telemetry.text;
                if (std::strncmp(text, "Title: ", 7) == 0) {
                    copyText(state_.player.title, sizeof(state_.player.title), text + 7);
                } else if (std::strncmp(text, "Artist: ", 8) == 0) {
                    copyText(state_.player.artist, sizeof(state_.player.artist), text + 8);
                } else if (std::strncmp(text, "Album: ", 7) == 0) {
                    copyText(state_.player.album, sizeof(state_.player.album), text + 7);
                } else if (text[0] != '\0') {
                    copyText(state_.player.title, sizeof(state_.player.title), text);
                }
                dirty_ = true;
                full_ = false;
                break;
            }

            case AudioTelemetryType::Error:
                Logger::warn("app", "audio: %s", telemetry.text);
                setStatus(telemetry.text);
                dirty_ = true;
                full_ = false;
                break;

            case AudioTelemetryType::Ready:
                state_.audioReady = true;
                break;
        }
    }
}

void App::updateBattery() {
    const uint32_t now = millis();
    if ((now - lastBatteryMs_) < kBatterySampleMs) {
        return;
    }
    lastBatteryMs_ = now;
    state_.battery = battery_.update();
    if (state_.battery.valid && state_.battery.percent < 10) {
        setStatus("Low battery");
    }
    dirty_ = true;
    full_ = false;
}

void App::run() {
    for (;;) {
        InputEvent event;
        if (xQueueReceive(gRuntime.inputQueue, &event, pdMS_TO_TICKS(100)) == pdTRUE) {
            handleInput(event);
        }
        handleTelemetry();
        updateBattery();

        if (dirty_) {
            ui_.render(state_, tracks_, full_);
            dirty_ = false;
            full_ = false;
        }
    }
}

}  // namespace ipsb
