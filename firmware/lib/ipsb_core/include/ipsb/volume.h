#pragma once

#include <cstdint>

namespace ipsb {

// ESP32-audioI2S volume range is 0..21 (default 21).
constexpr uint8_t kVolumeMaxStep = 21;
constexpr uint8_t kVolumeStartupStep = 4;  // ~19 % (4/21)
constexpr uint8_t kVolumeSafeMaxStep = 18; // firmware cap, ~86 %

uint8_t clampVolume(uint8_t step, uint8_t maxStep = kVolumeSafeMaxStep);
uint8_t volumePercent(uint8_t step);

}  // namespace ipsb
