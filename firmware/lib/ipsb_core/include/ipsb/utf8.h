#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace ipsb {

size_t utf8Length(const char* text);

// Largest prefix length <= maxBytes that ends on a UTF-8 code point boundary.
size_t utf8SafeTruncate(const char* text, size_t maxBytes);

// Returns the original string when it fits; otherwise a safe prefix + "...".
std::string utf8Ellipsize(const std::string& text, size_t maxBytes);

// Russian legacy tags often store CP1251 while claiming ISO-8859-1.
std::string cp1251ToUtf8(const uint8_t* data, size_t length);
std::string latin1ToUtf8(const uint8_t* data, size_t length);

}  // namespace ipsb
