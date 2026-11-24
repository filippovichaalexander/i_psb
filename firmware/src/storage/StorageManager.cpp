#include "storage/StorageManager.h"

#include <strings.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

#include "config.h"
#include "system/Logger.h"

namespace ipsb {
namespace {

bool hasAudioExtension(const char* name) {
    const char* dot = std::strrchr(name, '.');
    if (dot == nullptr) {
        return false;
    }
    return strcasecmp(dot, ".mp3") == 0 || strcasecmp(dot, ".flac") == 0 ||
           strcasecmp(dot, ".wav") == 0 || strcasecmp(dot, ".aac") == 0 ||
           strcasecmp(dot, ".m4a") == 0 || strcasecmp(dot, ".ogg") == 0 ||
           strcasecmp(dot, ".opus") == 0;
}

void copyBounded(char* dst, size_t dstSize, const char* src) {
    if (dst == nullptr || dstSize == 0) {
        return;
    }
    std::strncpy(dst, src ? src : "", dstSize - 1);
    dst[dstSize - 1] = '\0';
}

}  // namespace

bool StorageManager::begin() {
    ready_ = false;
    copyBounded(error_, sizeof(error_), "not mounted");

    sdSpi_.begin(pins::kSdSck, pins::kSdMiso, pins::kSdMosi, pins::kSdCs);
    if (!SD.begin(pins::kSdCs, sdSpi_, pins::kSdSpiHz)) {
        copyBounded(error_, sizeof(error_), "SD.begin failed");
        Logger::error("sd", "mount failed (CS=%d SCK=%d MISO=%d MOSI=%d)", pins::kSdCs,
                      pins::kSdSck, pins::kSdMiso, pins::kSdMosi);
        return false;
    }

    ready_ = true;
    copyBounded(error_, sizeof(error_), "ok");
    Logger::info("sd", "mounted: total=%.1f MB used=%.1f MB",
                 static_cast<double>(SD.totalBytes()) / (1024.0 * 1024.0),
                 static_cast<double>(SD.usedBytes()) / (1024.0 * 1024.0));
    ensureMusicDir();
    return true;
}

bool StorageManager::ensureMusicDir() {
    if (!ready_) {
        return false;
    }
    if (!SD.exists(kMusicDir) && !SD.mkdir(kMusicDir)) {
        Logger::warn("sd", "cannot create %s", kMusicDir);
        return false;
    }
    return true;
}

size_t StorageManager::scanMusic(std::vector<TrackEntry>& out, size_t maxTracks) {
    out.clear();
    if (!ready_) {
        return 0;
    }

    File dir = SD.open(kMusicDir);
    if (!dir) {
        copyBounded(error_, sizeof(error_), "open /Music failed");
        Logger::error("sd", "cannot open %s", kMusicDir);
        return 0;
    }

    File entry = dir.openNextFile();
    while (entry && out.size() < maxTracks) {
        if (!entry.isDirectory()) {
            const char* name = entry.name();
            if (name != nullptr && hasAudioExtension(name)) {
                TrackEntry track{};
                if (name[0] == '/') {
                    copyBounded(track.path, sizeof(track.path), name);
                } else {
                    std::snprintf(track.path, sizeof(track.path), "%s/%s", kMusicDir, name);
                }
                copyBounded(track.name, sizeof(track.name), name);
                // Strip a directory prefix and the extension for display.
                char* slash = std::strrchr(track.name, '/');
                if (slash != nullptr) {
                    std::memmove(track.name, slash + 1, std::strlen(slash + 1) + 1);
                }
                char* dot = std::strrchr(track.name, '.');
                if (dot != nullptr) {
                    *dot = '\0';
                }
                track.size = static_cast<uint32_t>(entry.size());
                out.push_back(track);
            }
        }
        entry.close();
        entry = dir.openNextFile();
    }
    dir.close();

    std::sort(out.begin(), out.end(), [](const TrackEntry& a, const TrackEntry& b) {
        return strcasecmp(a.name, b.name) < 0;
    });

    Logger::info("sd", "music scan: %u tracks", static_cast<unsigned>(out.size()));
    return out.size();
}

uint64_t StorageManager::totalBytes() const {
    return ready_ ? SD.totalBytes() : 0;
}

uint64_t StorageManager::usedBytes() const {
    return ready_ ? SD.usedBytes() : 0;
}

}  // namespace ipsb
