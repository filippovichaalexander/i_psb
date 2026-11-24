#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace ipsb {

struct Id3Tags {
  std::string title;
  std::string artist;
  std::string album;
  bool found = false;

  bool empty() const { return title.empty() && artist.empty() && album.empty(); }
};

// Parses ID3v2.3/2.4 text frames (TIT2, TPE1, TALB) from an in-memory buffer.
// ID3v2.2 and exotic frames are ignored; the caller can fall back to a filename.
Id3Tags parseId3v2(const uint8_t* data, size_t length);

}  // namespace ipsb
