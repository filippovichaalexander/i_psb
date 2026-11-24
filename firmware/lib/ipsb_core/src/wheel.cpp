#include "ipsb/wheel.h"

#include <cmath>

namespace ipsb {

WheelDecoder::WheelDecoder(uint8_t sectors) : sectors_(sectors == 0 ? 8 : sectors) {}

float WheelDecoder::sectorFromMask(uint8_t mask) const {
    int active[8];
    int count = 0;
    for (int i = 0; i < sectors_ && i < 8; ++i) {
        if (mask & (1u << i)) {
            active[count++] = i;
        }
    }
    if (count == 0) {
        return -1.0f;
    }
    if (count == 1) {
        return static_cast<float>(active[0]);
    }

    // Find the largest gap between active sectors. The active arc is the
    // complement, so its midpoint is the centroid we want.
    int bestGap = -1;
    int bestIndex = 0;
    for (int i = 0; i < count; ++i) {
        const int next = (i + 1 < count) ? active[i + 1] : active[0] + sectors_;
        const int gap = next - active[i];
        if (gap > bestGap) {
            bestGap = gap;
            bestIndex = i;
        }
    }

    const int start = (bestIndex + 1 < count) ? active[bestIndex + 1] : active[0] + sectors_;
    float sum = 0.0f;
    for (int i = 0; i < count; ++i) {
        int sector = active[i];
        while (sector < start) {
            sector += sectors_;
        }
        sum += static_cast<float>(sector);
    }
    const float centroid = sum / static_cast<float>(count);
    return std::fmod(centroid, static_cast<float>(sectors_));
}

float WheelDecoder::feed(uint8_t activeMask, uint32_t nowMs) {
    const float sector = sectorFromMask(activeMask);
    if (sector < 0.0f) {
        if (lastSector_ >= 0.0f && (nowMs - lastChangeMs_) > 150u) {
            lastSector_ = -1.0f;
        }
        return 0.0f;
    }

    lastChangeMs_ = nowMs;
    if (lastSector_ < 0.0f) {
        lastSector_ = sector;
        return 0.0f;
    }

    float delta = sector - lastSector_;
    const float half = static_cast<float>(sectors_) / 2.0f;
    while (delta > half) {
        delta -= static_cast<float>(sectors_);
    }
    while (delta < -half) {
        delta += static_cast<float>(sectors_);
    }
    lastSector_ = sector;

    if (delta > 3.0f || delta < -3.0f) {
        return 0.0f;
    }
    return delta;
}

void WheelDecoder::reset() {
    lastSector_ = -1.0f;
    lastChangeMs_ = 0;
}

}  // namespace ipsb
