#pragma once

namespace ipsb {

// M3: STA/AP portal, HTTP API and LittleFS uploader live here.
class WifiPortal {
  public:
    bool begin();
    void task();
};

}  // namespace ipsb
