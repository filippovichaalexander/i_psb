#pragma once

#include <cstdint>
#include <string>

namespace ipsb {

std::string formatTime(uint32_t seconds);
std::string formatBytes(uint64_t bytes);
std::string formatPercent(uint8_t percent);

}  // namespace ipsb
