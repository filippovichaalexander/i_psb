#pragma once

#include <cstddef>
#include <cstdint>

namespace ipsb {

// ---------------------------------------------------------------------------
// Pin map. See docs/02-pinout.md for the rationale and the original spec.
// Two pins differ from the v2.2 document:
//   BAT_ADC 42 -> 10  (GPIO42 has no ADC on ESP32-S3)
//   EPD_CS  10 -> 21  (frees ADC1_CH9)
// ---------------------------------------------------------------------------
namespace pins {

// E-Ink, FSPI
constexpr int kEpdMosi = 11;
constexpr int kEpdSck = 12;
constexpr int kEpdCs = 21;
constexpr int kEpdDc = 13;
constexpr int kEpdRst = 14;
constexpr int kEpdBusy = 15;

// microSD, HSPI
constexpr int kSdMosi = 7;
constexpr int kSdMiso = 8;
constexpr int kSdSck = 6;
constexpr int kSdCs = 9;

// I2S DAC
constexpr int kI2sDin = 17;
constexpr int kI2sBclk = 18;
constexpr int kI2sLrck = 16;
constexpr int kI2sMclk = -1;  // optional: 47/48 if the board exposes them

// TTP229 2-wire
constexpr int kTtpScl = 4;
constexpr int kTtpSdo = 5;

// Buttons (active low, INPUT_PULLUP)
constexpr int kBtnMenu = 1;
constexpr int kBtnPlay = 2;
constexpr int kBtnPrev = 38;
constexpr int kBtnNext = 39;
constexpr int kBtnSelect = 40;

// Haptics and battery
constexpr int kVibro = 41;
constexpr int kBatteryAdc = 10;  // ADC1_CH9

constexpr int kEpdSpiHz = 4000000;
constexpr int kSdSpiHz = 10000000;

}  // namespace pins

// Runtime tuning
constexpr size_t kPathMax = 160;
constexpr size_t kTitleMax = 96;
constexpr size_t kMaxTracks = 200;
constexpr uint8_t kSectorCount = 8;
constexpr uint32_t kInputPollMs = 10;
constexpr uint32_t kWheelGapResetMs = 150;
constexpr uint32_t kButtonDebounceMs = 20;
constexpr uint32_t kButtonLongPressMs = 700;
constexpr uint32_t kHapticPulseMs = 15;
constexpr uint32_t kBatterySampleMs = 5000;
constexpr uint32_t kTelemetryPeriodMs = 500;
constexpr uint32_t kProgressRefreshMs = 10000;
constexpr uint32_t kUiMinRenderMs = 250;
constexpr uint8_t kFullRefreshEvery = 30;

constexpr const char* kMusicDir = "/Music";
constexpr const char* kConfigDir = "/config";
constexpr const char* kLogDir = "/logs";

}  // namespace ipsb
