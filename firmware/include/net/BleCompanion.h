#pragma once

namespace ipsb {

// M4: NimBLE GATT control, queue and 1-bit album art live here.
class BleCompanion {
  public:
    bool begin();
    void task();
};

}  // namespace ipsb
