#pragma once

#include <cstdint>

namespace ipsb {

// Converts an 8-sector capacitive mask into a rotation delta.
//
// TTP229 periodically re-calibrates and may drop a held finger. The decoder
// therefore reacts to transitions between sectors, not to absolute contact.
// Multi-key masks are reduced to the centre of the active arc.
class WheelDecoder {
 public:
  explicit WheelDecoder(uint8_t sectors = 8);

  // Returns fractional ticks: +1 = one sector clockwise, -1 = counter-clockwise.
  // A gap of more than 3 sectors is treated as noise and returns 0.
  float feed(uint8_t activeMask, uint32_t nowMs);

  void reset();
  float lastSector() const { return lastSector_; }

 private:
  float sectorFromMask(uint8_t mask) const;

  uint8_t sectors_;
  float lastSector_ = -1.0f;
  uint32_t lastChangeMs_ = 0;
};

}  // namespace ipsb
