#include "ipsb/volume.h"

namespace ipsb {

uint8_t clampVolume(uint8_t step, uint8_t maxStep) {
    const uint8_t cap = maxStep > kVolumeMaxStep ? kVolumeMaxStep : maxStep;
    return step > cap ? cap : step;
}

uint8_t volumePercent(uint8_t step) {
    const uint8_t capped = clampVolume(step, kVolumeMaxStep);
    return static_cast<uint8_t>((static_cast<uint16_t>(capped) * 100U) / kVolumeMaxStep);
}

}  // namespace ipsb
