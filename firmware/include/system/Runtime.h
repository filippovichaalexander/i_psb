#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "types.h"

namespace ipsb {

struct Runtime {
    QueueHandle_t inputQueue = nullptr;
    QueueHandle_t audioCmdQueue = nullptr;
    QueueHandle_t telemetryQueue = nullptr;
};

extern Runtime gRuntime;

}  // namespace ipsb
