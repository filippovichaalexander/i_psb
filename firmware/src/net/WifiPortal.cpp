#include "net/WifiPortal.h"

#include "system/Logger.h"

namespace ipsb {

bool WifiPortal::begin() {
    Logger::info("wifi", "not implemented in M0 (planned M3)");
    return false;
}

void WifiPortal::task() {}

}  // namespace ipsb
