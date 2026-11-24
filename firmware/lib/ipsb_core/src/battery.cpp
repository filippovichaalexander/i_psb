#include "ipsb/battery.h"

#include <cstddef>

namespace ipsb {
namespace {

struct CurvePoint {
    uint16_t mv;
    uint8_t percent;
};

// Coarse Li-Po curve. Interpolation is linear between points.
constexpr CurvePoint kCurve[] = {
    {3250, 0}, {3500, 10}, {3650, 25}, {3750, 45}, {3850, 65}, {3950, 80}, {4050, 92}, {4180, 100},
};

constexpr size_t kCurveSize = sizeof(kCurve) / sizeof(kCurve[0]);

}  // namespace

uint8_t voltageToPercent(uint16_t millivolts) {
    if (millivolts <= kCurve[0].mv) {
        return 0;
    }
    if (millivolts >= kCurve[kCurveSize - 1].mv) {
        return 100;
    }

    for (size_t i = 1; i < kCurveSize; ++i) {
        const CurvePoint& lo = kCurve[i - 1];
        const CurvePoint& hi = kCurve[i];
        if (millivolts <= hi.mv) {
            const uint32_t spanMv = hi.mv - lo.mv;
            const uint32_t spanPct = hi.percent - lo.percent;
            const uint32_t offsetMv = millivolts - lo.mv;
            return static_cast<uint8_t>(lo.percent + (offsetMv * spanPct) / spanMv);
        }
    }
    return 100;
}

uint16_t adcMillivoltsToBatteryMillivolts(uint16_t adcMillivolts, uint16_t rTopK,
                                          uint16_t rBottomK) {
    if (rBottomK == 0) {
        return 0;
    }
    const uint32_t total = static_cast<uint32_t>(rTopK) + rBottomK;
    const uint32_t batteryMv = static_cast<uint32_t>(adcMillivolts) * total / rBottomK;
    return batteryMv > 0xFFFF ? 0xFFFF : static_cast<uint16_t>(batteryMv);
}

float EmaFilter::update(float sample) {
    if (!initialized_) {
        value_ = sample;
        initialized_ = true;
        return value_;
    }
    value_ = alpha_ * sample + (1.0f - alpha_) * value_;
    return value_;
}

void EmaFilter::reset(float value) {
    value_ = value;
    initialized_ = true;
}

}  // namespace ipsb
