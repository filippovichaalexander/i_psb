#pragma once

#include <cstdint>

namespace ipsb {

// Li-Po discharge curve for a single cell, in millivolts.
// Values are deliberately coarse: the ADC sees load sag, not open-circuit voltage.
// 3.25 V = 0 %, 4.18 V = 100 %.
uint8_t voltageToPercent(uint16_t millivolts);

// Undo a resistor divider. Default is the 100K/100K divider used on the board.
uint16_t adcMillivoltsToBatteryMillivolts(uint16_t adcMillivolts,
                                         uint16_t rTopK = 100,
                                         uint16_t rBottomK = 100);

// Exponential moving average with a cold-start snap.
class EmaFilter {
 public:
  explicit EmaFilter(float alpha = 0.2f) : alpha_(alpha) {}

  float update(float sample);
  float value() const { return value_; }
  void reset(float value = 0.0f);

 private:
  float alpha_;
  float value_ = 0.0f;
  bool initialized_ = false;
};

}  // namespace ipsb
