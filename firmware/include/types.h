#pragma once

#include <cstddef>
#include <cstdint>

#include "config.h"
#include "ipsb/playlist.h"
#include "ipsb/volume.h"

namespace ipsb {

enum class Screen : uint8_t {
    Boot = 0,
    Menu,
    Browse,
    NowPlaying,
    Settings,
    About,
    Notice,
    Error,
};

enum class ButtonId : uint8_t {
    Menu = 0,
    Play,
    Prev,
    Next,
    Select,
};

enum class InputType : uint8_t {
    Button = 0,
    Wheel,
};

enum class PressType : uint8_t {
    Short = 0,
    Long,
};

enum class AudioCmdType : uint8_t {
    Play = 0,
    PauseResume,
    Stop,
    Seek,
    SetVolume,
};

enum class AudioTelemetryType : uint8_t {
    Tick = 0,
    TrackEnded,
    TagData,
    Error,
    Ready,
};

struct InputEvent {
    InputType type = InputType::Button;
    ButtonId button = ButtonId::Menu;
    PressType press = PressType::Short;
    int16_t value = 0;  // wheel ticks
};

struct AudioCommand {
    AudioCmdType type = AudioCmdType::Play;
    uint32_t value = 0;
    char path[kPathMax] = {};
};

struct AudioTelemetry {
    AudioTelemetryType type = AudioTelemetryType::Tick;
    uint32_t positionSec = 0;
    uint32_t durationSec = 0;
    uint32_t bitrate = 0;
    uint32_t sampleRate = 0;
    uint8_t bits = 0;
    uint8_t channels = 0;
    bool running = false;
    char text[kTitleMax] = {};
};

struct TrackEntry {
    char path[kPathMax] = {};
    char name[kTitleMax] = {};
    uint32_t size = 0;
};

struct BatteryState {
    uint16_t millivolts = 0;
    uint8_t percent = 0;
    bool valid = false;
};

struct PlayerState {
    bool playing = false;
    uint32_t positionSec = 0;
    uint32_t durationSec = 0;
    uint32_t bitrate = 0;
    uint32_t sampleRate = 0;
    uint8_t bits = 0;
    char title[kTitleMax] = {};
    char artist[kTitleMax] = {};
    char album[kTitleMax] = {};
};

struct AppState {
    Screen screen = Screen::Boot;
    uint8_t menuIndex = 0;
    uint8_t browseIndex = 0;
    uint8_t browseTop = 0;
    uint8_t volume = kVolumeStartupStep;
    RepeatMode repeatMode = RepeatMode::Off;
    BatteryState battery = {};
    PlayerState player = {};
    bool sdReady = false;
    bool audioReady = false;
    char status[64] = {};
};

}  // namespace ipsb
