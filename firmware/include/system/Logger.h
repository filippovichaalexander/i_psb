#pragma once

#include <cstdint>

namespace ipsb {

enum class LogLevel : uint8_t {
    Info = 0,
    Warn,
    Error,
};

class Logger {
  public:
    static void begin(uint32_t baud = 115200);
    static void bootInfo();

    static void info(const char* tag, const char* fmt, ...);
    static void warn(const char* tag, const char* fmt, ...);
    static void error(const char* tag, const char* fmt, ...);

  private:
    static void log(LogLevel level, const char* tag, const char* fmt, ...);
};

}  // namespace ipsb
