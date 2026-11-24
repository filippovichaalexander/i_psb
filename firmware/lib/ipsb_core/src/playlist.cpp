#include "ipsb/playlist.h"

namespace ipsb {

PlaylistCursor::PlaylistCursor(size_t count, RepeatMode mode, bool shuffle, uint32_t seed)
    : count_(count), mode_(mode), shuffle_(shuffle), rng_(seed ? seed : 1) {}

void PlaylistCursor::setCount(size_t count) {
  count_ = count;
  if (count_ == 0) {
    current_ = 0;
  } else if (current_ >= count_) {
    current_ = count_ - 1;
  }
}

void PlaylistCursor::setShuffle(bool enabled, uint32_t seed) {
  shuffle_ = enabled;
  rng_ = seed ? seed : 1;
}

void PlaylistCursor::setCurrent(size_t index) {
  if (count_ == 0) {
    current_ = 0;
    return;
  }
  current_ = index >= count_ ? count_ - 1 : index;
}

size_t PlaylistCursor::randomIndex() {
  if (count_ <= 1) {
    return 0;
  }
  // xorshift32: deterministic and cheap; tests use a fixed seed.
  rng_ ^= rng_ << 13;
  rng_ ^= rng_ >> 17;
  rng_ ^= rng_ << 5;
  size_t candidate = static_cast<size_t>(rng_ % count_);
  if (candidate == current_) {
    candidate = (candidate + 1) % count_;
  }
  return candidate;
}

bool PlaylistCursor::next() {
  if (count_ == 0) {
    return false;
  }
  if (mode_ == RepeatMode::One) {
    return true;
  }
  if (shuffle_) {
    current_ = randomIndex();
    return true;
  }
  if (current_ + 1 < count_) {
    ++current_;
    return true;
  }
  if (mode_ == RepeatMode::All) {
    current_ = 0;
    return true;
  }
  return false;
}

bool PlaylistCursor::previous() {
  if (count_ == 0) {
    return false;
  }
  if (mode_ == RepeatMode::One) {
    return true;
  }
  if (shuffle_) {
    current_ = randomIndex();
    return true;
  }
  if (current_ > 0) {
    --current_;
    return true;
  }
  if (mode_ == RepeatMode::All) {
    current_ = count_ - 1;
    return true;
  }
  return false;
}

}  // namespace ipsb
