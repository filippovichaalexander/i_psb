#include "audio/AudioEngine.h"

#include <SD.h>

#include <cstring>

#include "config.h"
#include "system/Logger.h"
#include "system/Runtime.h"

namespace ipsb {
namespace {

void copyText(char* dst, size_t dstSize, const char* src) {
    if (dst == nullptr || dstSize == 0) {
        return;
    }
    std::strncpy(dst, src ? src : "", dstSize - 1);
    dst[dstSize - 1] = '\0';
}

}  // namespace

bool AudioEngine::begin() {
    if (!audio_.setPinout(pins::kI2sBclk, pins::kI2sLrck, pins::kI2sDin, pins::kI2sMclk)) {
        Logger::error("audio", "setPinout failed (BCLK=%d LRCK=%d DIN=%d)", pins::kI2sBclk,
                      pins::kI2sLrck, pins::kI2sDin);
        return false;
    }

    audio_.setVolume(volume_);
    audio_.setAudioTaskCore(0);
    Audio::audio_info_callback = [](Audio::msg_t msg) { AudioEngine::onInfo(msg); };

    ready_ = true;
    Logger::info("audio", "I2S ready: BCLK=%d LRCK=%d DIN=%d MCLK=%d volume=%u/21", pins::kI2sBclk,
                 pins::kI2sLrck, pins::kI2sDin, pins::kI2sMclk, static_cast<unsigned>(volume_));
    publish(AudioTelemetryType::Ready);
    return true;
}

void AudioEngine::setVolume(uint8_t step) {
    volume_ = clampVolume(step);
    audio_.setVolume(volume_);
}

void AudioEngine::task() {
    for (;;) {
        AudioCommand command;
        if (xQueueReceive(gRuntime.audioCmdQueue, &command, 0) == pdTRUE) {
            handleCommand(command);
        }

        audio_.loop();

        const uint32_t now = millis();
        if ((now - lastTelemetryMs_) >= kTelemetryPeriodMs) {
            lastTelemetryMs_ = now;
            publish(AudioTelemetryType::Tick);
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void AudioEngine::handleCommand(const AudioCommand& command) {
    switch (command.type) {
        case AudioCmdType::Play:
            if (!audio_.connecttoFS(SD, command.path)) {
                Logger::error("audio", "cannot play %s", command.path);
                publishError("play failed");
            }
            break;
        case AudioCmdType::PauseResume:
            audio_.pauseResume();
            break;
        case AudioCmdType::Stop:
            audio_.stopSong();
            break;
        case AudioCmdType::Seek:
            audio_.setAudioFilePosition(command.value);
            break;
        case AudioCmdType::SetVolume:
            setVolume(static_cast<uint8_t>(command.value));
            break;
    }
}

void AudioEngine::publish(AudioTelemetryType type) {
    if (gRuntime.telemetryQueue == nullptr) {
        return;
    }
    AudioTelemetry telemetry{};
    telemetry.type = type;
    telemetry.positionSec = audio_.getAudioCurrentTime();
    telemetry.durationSec = audio_.getAudioFileDuration();
    telemetry.bitrate = audio_.getBitRate();
    telemetry.sampleRate = audio_.getSampleRate();
    telemetry.bits = audio_.getBitsPerSample();
    telemetry.channels = audio_.getChannels();
    telemetry.running = audio_.isRunning();
    xQueueSend(gRuntime.telemetryQueue, &telemetry, 0);
}

void AudioEngine::publishError(const char* text) {
    if (gRuntime.telemetryQueue == nullptr) {
        return;
    }
    AudioTelemetry telemetry{};
    telemetry.type = AudioTelemetryType::Error;
    copyText(telemetry.text, sizeof(telemetry.text), text);
    xQueueSend(gRuntime.telemetryQueue, &telemetry, 0);
}

void AudioEngine::onInfo(Audio::msg_t msg) {
    if (gRuntime.telemetryQueue == nullptr) {
        return;
    }
    AudioTelemetry telemetry{};
    switch (msg.e) {
        case Audio::evt_eof:
            telemetry.type = AudioTelemetryType::TrackEnded;
            break;
        case Audio::evt_id3data:
            telemetry.type = AudioTelemetryType::TagData;
            copyText(telemetry.text, sizeof(telemetry.text), msg.msg);
            break;
        case Audio::evt_log:
            telemetry.type = AudioTelemetryType::Error;
            copyText(telemetry.text, sizeof(telemetry.text), msg.msg);
            break;
        default:
            return;
    }
    xQueueSend(gRuntime.telemetryQueue, &telemetry, 0);
}

}  // namespace ipsb
