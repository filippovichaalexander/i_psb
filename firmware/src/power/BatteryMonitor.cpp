#include "power/BatteryMonitor.h"

#include <Arduino.h>

#include "config.h"
#include "system/Logger.h"

namespace ipsb {

void BatteryMonitor::begin() {
    pinMode(pins::kBatteryAdc, INPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(pins::kBatteryAdc, ADC_11db);
    state_ = update(true);
    Logger::info("battery", "battery=%umV pct=%u", state_.millivolts, state_.percent);
}

uint16_t BatteryMonitor::readBatteryMillivolts() {
    constexpr int kSamples = 32;
    uint32_t sum = 0;
    for (int i = 0; i < kSamples; ++i) {
        sum += analogReadMilliVolts(pins::kBatteryAdc);
    }
    const uint32_t adcMv = sum / kSamples;
    return adcMillivoltsToBatteryMillivolts(static_cast<uint16_t>(adcMv));
}

BatteryState BatteryMonitor::update(bool force) {
    const uint32_t now = millis();
    if (!force && (now - lastSampleMs_) < kBatterySampleMs) {
        return state_;
    }
    lastSampleMs_ = now;

    const float filtered = filter_.update(static_cast<float>(readBatteryMillivolts()));
    state_.millivolts = static_cast<uint16_t>(filtered + 0.5f);
    state_.percent = voltageToPercent(state_.millivolts);
    state_.valid = state_.millivolts > 2500;
    return state_;
}

}  // namespace ipsb
