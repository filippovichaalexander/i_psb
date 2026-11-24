#pragma once

#include <cstddef>
#include <cstdint>

namespace ipsb {

enum class RepeatMode : uint8_t {
    Off = 0,
    One = 1,
    All = 2,
};

class PlaylistCursor {
  public:
    explicit PlaylistCursor(size_t count = 0, RepeatMode mode = RepeatMode::Off,
                            bool shuffle = false, uint32_t seed = 0x12345678u);

    void setCount(size_t count);
    void setRepeatMode(RepeatMode mode) {
        mode_ = mode;
    }
    void setShuffle(bool enabled, uint32_t seed = 0x12345678u);
    void setCurrent(size_t index);

    size_t current() const {
        return current_;
    }
    size_t count() const {
        return count_;
    }
    RepeatMode repeatMode() const {
        return mode_;
    }
    bool shuffle() const {
        return shuffle_;
    }

    // Returns false only when playback should stop (repeat off at the end).
    bool next();
    bool previous();

  private:
    size_t randomIndex();

    size_t count_ = 0;
    size_t current_ = 0;
    RepeatMode mode_ = RepeatMode::Off;
    bool shuffle_ = false;
    uint32_t rng_ = 0x12345678u;
};

}  // namespace ipsb
