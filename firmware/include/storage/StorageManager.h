#pragma once

#include <SD.h>
#include <SPI.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "types.h"

namespace ipsb {

class StorageManager {
  public:
    bool begin();
    bool ready() const {
        return ready_;
    }
    const char* error() const {
        return error_;
    }

    size_t scanMusic(std::vector<TrackEntry>& out, size_t maxTracks);
    uint64_t totalBytes() const;
    uint64_t usedBytes() const;

  private:
    bool ensureMusicDir();

    SPIClass sdSpi_{HSPI};
    bool ready_ = false;
    char error_[64] = "not mounted";
};

}  // namespace ipsb
