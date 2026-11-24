#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "app/App.h"
#include "audio/AudioEngine.h"
#include "config.h"
#include "input/InputManager.h"
#include "net/BleCompanion.h"
#include "net/WifiPortal.h"
#include "power/BatteryMonitor.h"
#include "storage/StorageManager.h"
#include "system/Logger.h"
#include "system/Runtime.h"
#include "types.h"
#include "ui/UiRenderer.h"

namespace ipsb {

Runtime gRuntime{};

StorageManager gStorage;
BatteryMonitor gBattery;
AudioEngine gAudio;
UiRenderer gUi;
InputManager gInput;
WifiPortal gWifi;
BleCompanion gBle;
App gApp(gStorage, gBattery, gAudio, gUi);

}  // namespace ipsb

namespace {

void audioTask(void*) {
    ipsb::gAudio.task();
}

void inputTask(void*) {
    ipsb::gInput.task();
}

void uiTask(void*) {
    ipsb::gApp.run();
}

}  // namespace

void setup() {
    using namespace ipsb;

    Logger::begin();
    Logger::bootInfo();

    gRuntime.inputQueue = xQueueCreate(16, sizeof(InputEvent));
    gRuntime.audioCmdQueue = xQueueCreate(8, sizeof(AudioCommand));
    gRuntime.telemetryQueue = xQueueCreate(8, sizeof(AudioTelemetry));
    if (gRuntime.inputQueue == nullptr || gRuntime.audioCmdQueue == nullptr ||
        gRuntime.telemetryQueue == nullptr) {
        Logger::error("main", "queue allocation failed");
    }

    gStorage.begin();
    gBattery.begin();
    gUi.begin();
    gUi.splash("i_psb", "E-Ink Hi-Fi player");
    gAudio.begin();
    gInput.begin();
    gWifi.begin();
    gBle.begin();
    gApp.begin();

    // ESP32-audioI2S creates its own priority-2 DMA task in setPinout().
    // Keep the feeder below it: a long SD read must never starve I2S.
    BaseType_t ok = pdPASS;
    ok &= xTaskCreatePinnedToCore(audioTask, "audio-feed", 8192, nullptr, 1, nullptr, 0);
    ok &= xTaskCreatePinnedToCore(inputTask, "input", 4096, nullptr, 3, nullptr, 1);
    ok &= xTaskCreatePinnedToCore(uiTask, "ui", 8192, nullptr, 2, nullptr, 1);
    if (ok != pdPASS) {
        Logger::error("main", "task creation failed");
    }
    Logger::info("main", "tasks started");
}

void loop() {
    // All work lives in FreeRTOS tasks. Keep the Arduino loop empty on purpose.
    vTaskDelay(pdMS_TO_TICKS(1000));
}
