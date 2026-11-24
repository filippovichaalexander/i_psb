#include "net/BleCompanion.h"

#include "system/Logger.h"

namespace ipsb {

bool BleCompanion::begin() {
    Logger::info("ble", "not implemented in M0 (planned M4)");
    return false;
}

void BleCompanion::task() {}

}  // namespace ipsb
