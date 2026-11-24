#include "system/Logger.h"

#include <Arduino.h>
#include <esp_system.h>

#include <cstdarg>
#include <cstdio>

#include "ipsb/version.h"

namespace ipsb {
namespace {

const char* levelName(LogLevel level) {
    switch (level) {
        case LogLevel::Warn:
            return "w";
        case LogLevel::Error:
            return "e";
        case LogLevel::Info:
        default:
            return "i";
    }
}

const char* resetReasonName(esp_reset_reason_t reason) {
    switch (reason) {
        case ESP_RST_POWERON:
            return "power-on";
        case ESP_RST_EXT:
            return "external";
        case ESP_RST_SW:
            return "software";
        case ESP_RST_PANIC:
            return "panic";
        case ESP_RST_INT_WDT:
            return "int-wdt";
        case ESP_RST_TASK_WDT:
            return "task-wdt";
        case ESP_RST_WDT:
            return "wdt";
        case ESP_RST_DEEPSLEEP:
            return "deepsleep";
        case ESP_RST_BROWNOUT:
            return "brownout";
        case ESP_RST_SDIO:
            return "sdio";
        default:
            return "unknown";
    }
}

}  // namespace

void Logger::begin(uint32_t baud) {
    Serial.begin(baud);
    const uint32_t start = millis();
    while (!Serial && (millis() - start) < 1500) {
        delay(10);
    }
}

void Logger::log(LogLevel level, const char* tag, const char* fmt, ...) {
    char message[192];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    Serial.printf("[%lu][%s][%s] %s\n", static_cast<unsigned long>(millis()), levelName(level),
                  tag ? tag : "-", message);
}

void Logger::info(const char* tag, const char* fmt, ...) {
    char message[192];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
    Serial.printf("[%lu][i][%s] %s\n", static_cast<unsigned long>(millis()), tag ? tag : "-",
                  message);
}

void Logger::warn(const char* tag, const char* fmt, ...) {
    char message[192];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
    Serial.printf("[%lu][w][%s] %s\n", static_cast<unsigned long>(millis()), tag ? tag : "-",
                  message);
}

void Logger::error(const char* tag, const char* fmt, ...) {
    char message[192];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
    Serial.printf("[%lu][e][%s] %s\n", static_cast<unsigned long>(millis()), tag ? tag : "-",
                  message);
}

void Logger::bootInfo() {
    info("boot", "%s %s", kFirmwareName, kFirmwareVersion);
    info("boot", "reset=%s chip_rev=%u flash=%lu psram=%lu heap=%lu",
         resetReasonName(esp_reset_reason()), static_cast<unsigned>(ESP.getChipRevision()),
         static_cast<unsigned long>(ESP.getFlashChipSize()),
         static_cast<unsigned long>(ESP.getPsramSize()),
         static_cast<unsigned long>(ESP.getFreeHeap()));
}

}  // namespace ipsb
