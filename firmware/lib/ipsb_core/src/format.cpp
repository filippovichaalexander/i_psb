#include "ipsb/format.h"

#include <cstdio>

namespace ipsb {

std::string formatTime(uint32_t seconds) {
  char buffer[16];
  const uint32_t hours = seconds / 3600u;
  const uint32_t minutes = (seconds % 3600u) / 60u;
  const uint32_t secs = seconds % 60u;
  if (hours > 0) {
    std::snprintf(buffer, sizeof(buffer), "%lu:%02lu:%02lu",
                  static_cast<unsigned long>(hours),
                  static_cast<unsigned long>(minutes),
                  static_cast<unsigned long>(secs));
  } else {
    std::snprintf(buffer, sizeof(buffer), "%lu:%02lu",
                  static_cast<unsigned long>(minutes),
                  static_cast<unsigned long>(secs));
  }
  return std::string(buffer);
}

std::string formatBytes(uint64_t bytes) {
  char buffer[32];
  if (bytes >= (1024ull * 1024ull * 1024ull)) {
    std::snprintf(buffer, sizeof(buffer), "%.1f GB",
                  static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0));
  } else if (bytes >= (1024ull * 1024ull)) {
    std::snprintf(buffer, sizeof(buffer), "%.1f MB",
                  static_cast<double>(bytes) / (1024.0 * 1024.0));
  } else if (bytes >= 1024ull) {
    std::snprintf(buffer, sizeof(buffer), "%.1f KB", static_cast<double>(bytes) / 1024.0);
  } else {
    std::snprintf(buffer, sizeof(buffer), "%llu B",
                  static_cast<unsigned long long>(bytes));
  }
  return std::string(buffer);
}

std::string formatPercent(uint8_t percent) {
  char buffer[8];
  std::snprintf(buffer, sizeof(buffer), "%u%%", static_cast<unsigned>(percent));
  return std::string(buffer);
}

}  // namespace ipsb
