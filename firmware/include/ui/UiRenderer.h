#pragma once

#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>

#include <cstdint>
#include <vector>

#include "config.h"
#include "types.h"

namespace ipsb {

class UiRenderer {
  public:
    bool begin();
    void splash(const char* line1, const char* line2);
    void render(const AppState& state, const std::vector<TrackEntry>& tracks,
                bool forceFull = false);

  private:
    void drawScreen(const AppState& state, const std::vector<TrackEntry>& tracks);
    void drawStatusBar(const AppState& state);
    void drawMenu(const AppState& state);
    void drawBrowse(const AppState& state, const std::vector<TrackEntry>& tracks);
    void drawNowPlaying(const AppState& state);
    void drawSettings(const AppState& state);
    void drawAbout();
    void drawNotice(const AppState& state);
    void drawError(const AppState& state);

    void drawListRow(int y, const char* text, bool selected, int rowHeight = 18);
    void drawTextFit(const char* text, int x, int y, int maxWidth, bool large,
                     bool inverted = false);
    void drawText(const char* text, int x, int y, bool large, bool inverted);
    void setFont(bool large);

    GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display_{
        GxEPD2_213_B74(pins::kEpdCs, pins::kEpdDc, pins::kEpdRst, pins::kEpdBusy)};
    U8G2_FOR_ADAFRUIT_GFX u8g2_;

    int width_ = 250;
    int height_ = 122;
    Screen lastScreen_ = Screen::Boot;
    uint8_t partialCount_ = 0;
    uint32_t lastRenderMs_ = 0;
};

}  // namespace ipsb
