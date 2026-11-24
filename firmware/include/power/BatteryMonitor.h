#pragma once

#include <cstdint>

#include "ipsb/battery.h"
#include "types.h"

namespace ipsb {

class BatteryMonitor {
  public:
    void begin();
    BatteryState update(bool force = false);
    BatteryState state() const {
        return state_;
    }

  private:
    uint16_t readBatteryMillivolts();

    EmaFilter filter_{0.2f};
    BatteryState state_{};
    uint32_t lastSampleMs_ = 0;
};

}  // namespace ipsb
