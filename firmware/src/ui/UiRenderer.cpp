#include "ui/UiRenderer.h"

#include <Arduino.h>
#include <SPI.h>

#include <cstdio>
#include <cstring>
#include <string>

#include "ipsb/format.h"
#include "ipsb/utf8.h"
#include "ipsb/version.h"
#include "system/Logger.h"

namespace ipsb {
namespace {

constexpr int kStatusBottom = 18;

const char* repeatName(RepeatMode mode) {
    switch (mode) {
        case RepeatMode::One:
            return "One";
        case RepeatMode::All:
            return "All";
        case RepeatMode::Off:
        default:
            return "Off";
    }
}

}  // namespace

bool UiRenderer::begin() {
    SPI.begin(pins::kEpdSck, -1, pins::kEpdMosi, pins::kEpdCs);

    display_.init(115200, true, 2, false);
    display_.setRotation(1);
    width_ = display_.width();
    height_ = display_.height();

    u8g2_.begin(display_);
    u8g2_.setFontMode(1);
    u8g2_.setFontDirection(0);
    u8g2_.setForegroundColor(GxEPD_BLACK);
    u8g2_.setBackgroundColor(GxEPD_WHITE);

    Logger::info("epd", "panel %dx%d rotation=1", width_, height_);
    return true;
}

void UiRenderer::setFont(bool large) {
    u8g2_.setFont(large ? u8g2_font_10x20_t_cyrillic : u8g2_font_unifont_t_cyrillic);
}

void UiRenderer::drawText(const char* text, int x, int y, bool large, bool inverted) {
    setFont(large);
    u8g2_.setForegroundColor(inverted ? GxEPD_WHITE : GxEPD_BLACK);
    u8g2_.setBackgroundColor(inverted ? GxEPD_BLACK : GxEPD_WHITE);
    // U8g2_for_Adafruit_GFX 1.8 draws at the baseline; our layout uses top edges.
    const int baseline = y + u8g2_.getFontAscent();
    u8g2_.drawUTF8(static_cast<int16_t>(x), static_cast<int16_t>(baseline), text ? text : "");
    u8g2_.setForegroundColor(GxEPD_BLACK);
    u8g2_.setBackgroundColor(GxEPD_WHITE);
}

void UiRenderer::drawTextFit(const char* text, int x, int y, int maxWidth, bool large,
                             bool inverted) {
    setFont(large);
    const std::string source = text ? text : "";
    if (u8g2_.getUTF8Width(source.c_str()) <= maxWidth) {
        drawText(source.c_str(), x, y, large, inverted);
        return;
    }

    for (size_t limit = source.size(); limit > 0; --limit) {
        const std::string candidate = utf8Ellipsize(source, limit);
        if (u8g2_.getUTF8Width(candidate.c_str()) <= maxWidth) {
            drawText(candidate.c_str(), x, y, large, inverted);
            return;
        }
    }
}

void UiRenderer::drawListRow(int y, const char* text, bool selected, int rowHeight) {
    if (selected) {
        display_.fillRect(0, y - 1, width_, rowHeight, GxEPD_BLACK);
    }
    drawTextFit(text, 6, y, width_ - 12, false, selected);
}

void UiRenderer::drawStatusBar(const AppState& state) {
    char volume[24];
    std::snprintf(volume, sizeof(volume), "VOL %u%%", volumePercent(state.volume));
    drawText(volume, 4, 0, false, false);

    drawText("iPSB", 104, 0, false, false);

    char battery[24];
    if (state.battery.valid) {
        std::snprintf(battery, sizeof(battery), "%u%%", state.battery.percent);
    } else {
        std::snprintf(battery, sizeof(battery), "--");
    }
    drawText(battery, 190, 0, false, false);

    const int bx = width_ - 28;
    display_.drawRect(bx, 4, 22, 9, GxEPD_BLACK);
    display_.drawRect(bx + 22, 7, 2, 3, GxEPD_BLACK);
    if (state.battery.valid) {
        const int fill = (state.battery.percent * 20) / 100;
        if (fill > 0) {
            display_.fillRect(bx + 1, 5, fill, 7, GxEPD_BLACK);
        }
    }

    display_.drawFastHLine(0, kStatusBottom - 1, width_, GxEPD_BLACK);
}

void UiRenderer::drawMenu(const AppState& state) {
    static const char* kItems[] = {"Music",      "Now Playing", "Radio",
                                   "Wi-Fi Sync", "Settings",    "About"};
    constexpr int kItemCount = 6;
    constexpr int kVisible = 4;

    drawTextFit("iPSB", 8, 22, width_ - 16, true);

    int top = static_cast<int>(state.menuIndex) - 1;
    if (top < 0) {
        top = 0;
    }
    if (top > kItemCount - kVisible) {
        top = kItemCount - kVisible;
    }

    for (int i = 0; i < kVisible; ++i) {
        const int index = top + i;
        const int y = 44 + i * 18;
        drawListRow(y, kItems[index], index == state.menuIndex, 18);
    }
}

void UiRenderer::drawBrowse(const AppState& state, const std::vector<TrackEntry>& tracks) {
    char title[48];
    std::snprintf(title, sizeof(title), "Music (%u)", static_cast<unsigned>(tracks.size()));
    drawTextFit(title, 8, 20, width_ - 16, false);

    if (tracks.empty()) {
        drawTextFit(state.status[0] ? state.status : "No music found", 8, 46, width_ - 16, false);
        return;
    }

    constexpr int kVisible = 5;
    for (int i = 0; i < kVisible; ++i) {
        const size_t index = static_cast<size_t>(state.browseTop) + i;
        if (index >= tracks.size()) {
            break;
        }
        const int y = 39 + i * 16;
        drawListRow(y, tracks[index].name, index == state.browseIndex, 16);
    }
}

void UiRenderer::drawNowPlaying(const AppState& state) {
    drawTextFit(state.player.title[0] ? state.player.title : "No track", 8, 22, width_ - 16, true);
    drawTextFit(state.player.artist[0] ? state.player.artist : " ", 8, 44, width_ - 16, false);
    drawTextFit(state.player.album[0] ? state.player.album : " ", 8, 60, width_ - 16, false);

    const int px = 12;
    const int py = 80;
    const int pw = width_ - 24;
    const int ph = 6;
    display_.drawRect(px, py, pw, ph, GxEPD_BLACK);
    if (state.player.durationSec > 0) {
        uint32_t ratio =
            static_cast<uint32_t>((static_cast<uint64_t>(state.player.positionSec) * (pw - 2)) /
                                  state.player.durationSec);
        if (ratio > static_cast<uint32_t>(pw - 2)) {
            ratio = pw - 2;
        }
        if (ratio > 0) {
            display_.fillRect(px + 1, py + 1, static_cast<int>(ratio), ph - 2, GxEPD_BLACK);
        }
    }

    const std::string pos = formatTime(state.player.positionSec);
    const std::string dur = formatTime(state.player.durationSec);
    drawText(pos.c_str(), px, 90, false, false);
    drawText(dur.c_str(), width_ - px - u8g2_.getUTF8Width(dur.c_str()), 90, false, false);

    char bottom[64];
    if (state.player.sampleRate > 0) {
        std::snprintf(bottom, sizeof(bottom), "%.1f kHz / %u bit",
                      static_cast<double>(state.player.sampleRate) / 1000.0,
                      static_cast<unsigned>(state.player.bits));
    } else {
        std::snprintf(bottom, sizeof(bottom), "%s", state.player.playing ? "playing" : "stopped");
    }
    drawTextFit(bottom, px, 104, width_ - 24, false);

    if (!state.player.playing) {
        drawText("PAUSED", width_ - 78, 104, false, false);
    }
}

void UiRenderer::drawSettings(const AppState& state) {
    drawTextFit("Settings", 8, 20, width_ - 16, true);

    char repeat[32];
    std::snprintf(repeat, sizeof(repeat), "Repeat: %s", repeatName(state.repeatMode));
    char volume[32];
    std::snprintf(volume, sizeof(volume), "Volume: %u%%", volumePercent(state.volume));
    char battery[48];
    if (state.battery.valid) {
        std::snprintf(battery, sizeof(battery), "Battery: %.2f V  %u%%",
                      static_cast<double>(state.battery.millivolts) / 1000.0,
                      static_cast<unsigned>(state.battery.percent));
    } else {
        std::snprintf(battery, sizeof(battery), "Battery: --");
    }

    const char* items[4] = {repeat, volume, battery, "Back"};
    for (int i = 0; i < 4; ++i) {
        drawListRow(42 + i * 18, items[i], i == state.menuIndex, 18);
    }
}

void UiRenderer::drawAbout() {
    drawTextFit("i_psb", 8, 22, width_ - 16, true);
    drawTextFit("ESP32-S3 + PCM5102A + MAX97220", 8, 48, width_ - 16, false);
    drawTextFit("E-Ink 2.13\" 250x122", 8, 66, width_ - 16, false);
    drawTextFit("github.com/filippovichaalexander/i_psb", 8, 84, width_ - 16, false);
    drawTextFit("M0 skeleton - not a finished player", 8, 102, width_ - 16, false);
}

void UiRenderer::drawNotice(const AppState& state) {
    drawTextFit("Not yet", 8, 22, width_ - 16, true);
    drawTextFit(state.status[0] ? state.status : "Planned for a later milestone", 8, 50,
                width_ - 16, false);
    drawTextFit("Press MENU to go back", 8, 100, width_ - 16, false);
}

void UiRenderer::drawError(const AppState& state) {
    drawTextFit("Error", 8, 22, width_ - 16, true);
    drawTextFit(state.status[0] ? state.status : "Unknown error", 8, 50, width_ - 16, false);
    drawTextFit("Press MENU", 8, 100, width_ - 16, false);
}

void UiRenderer::drawScreen(const AppState& state, const std::vector<TrackEntry>& tracks) {
    drawStatusBar(state);

    switch (state.screen) {
        case Screen::Menu:
            drawMenu(state);
            break;
        case Screen::Browse:
            drawBrowse(state, tracks);
            break;
        case Screen::NowPlaying:
            drawNowPlaying(state);
            break;
        case Screen::Settings:
            drawSettings(state);
            break;
        case Screen::About:
            drawAbout();
            break;
        case Screen::Notice:
            drawNotice(state);
            break;
        case Screen::Error:
            drawError(state);
            break;
        case Screen::Boot:
        default:
            drawTextFit("Booting...", 8, 50, width_ - 16, true);
            break;
    }
}

void UiRenderer::render(const AppState& state, const std::vector<TrackEntry>& tracks,
                        bool forceFull) {
    const uint32_t now = millis();
    if (!forceFull && (now - lastRenderMs_) < kUiMinRenderMs) {
        return;
    }

    const bool full =
        forceFull || state.screen != lastScreen_ || partialCount_ >= kFullRefreshEvery;

    lastRenderMs_ = now;

    if (full) {
        display_.setFullWindow();
        display_.firstPage();
        do {
            display_.fillScreen(GxEPD_WHITE);
            drawScreen(state, tracks);
        } while (display_.nextPage());
        partialCount_ = 0;
        lastScreen_ = state.screen;
    } else {
        display_.setPartialWindow(0, 0, 248, 122);
        display_.firstPage();
        do {
            display_.fillRect(0, 0, 248, 122, GxEPD_WHITE);
            drawScreen(state, tracks);
        } while (display_.nextPage());
        ++partialCount_;
    }
}

void UiRenderer::splash(const char* line1, const char* line2) {
    display_.setFullWindow();
    display_.firstPage();
    do {
        display_.fillScreen(GxEPD_WHITE);
        drawTextFit(line1, 10, 22, width_ - 20, true);
        drawTextFit(line2, 10, 52, width_ - 20, false);
        drawTextFit("v" IPSB_FW_VERSION, 10, 100, width_ - 20, false);
    } while (display_.nextPage());

    partialCount_ = 0;
    lastScreen_ = Screen::Boot;
}

}  // namespace ipsb
