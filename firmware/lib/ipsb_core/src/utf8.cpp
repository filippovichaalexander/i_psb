#include "ipsb/utf8.h"

#include <cstring>

namespace ipsb {
namespace {

constexpr uint16_t kCp1251High[128] = {
    0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021,
    0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
    0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0x0098, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
    0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7,
    0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
    0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7,
    0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457,
    0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
    0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
    0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
    0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
    0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
    0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
    0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
    0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F,
};

void appendUtf8(std::string& out, uint32_t cp) {
  if (cp <= 0x7F) {
    out.push_back(static_cast<char>(cp));
  } else if (cp <= 0x7FF) {
    out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else if (cp <= 0xFFFF) {
    out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else {
    out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  }
}

}  // namespace

size_t utf8Length(const char* text) {
  if (text == nullptr) {
    return 0;
  }
  size_t count = 0;
  for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text); *p; ++p) {
    if ((*p & 0xC0) != 0x80) {
      ++count;
    }
  }
  return count;
}

size_t utf8SafeTruncate(const char* text, size_t maxBytes) {
  if (text == nullptr || maxBytes == 0) {
    return 0;
  }
  const size_t length = std::strlen(text);
  if (length <= maxBytes) {
    return length;
  }
  size_t pos = maxBytes;
  while (pos > 0 &&
         (static_cast<unsigned char>(text[pos]) & 0xC0) == 0x80) {
    --pos;
  }
  return pos;
}

std::string utf8Ellipsize(const std::string& text, size_t maxBytes) {
  if (text.size() <= maxBytes) {
    return text;
  }
  if (maxBytes <= 3) {
    return std::string(maxBytes, '.');
  }
  const size_t prefix = utf8SafeTruncate(text.c_str(), maxBytes - 3);
  return text.substr(0, prefix) + "...";
}

std::string cp1251ToUtf8(const uint8_t* data, size_t length) {
  std::string out;
  out.reserve(length * 2);
  if (data == nullptr) {
    return out;
  }
  for (size_t i = 0; i < length; ++i) {
    const uint8_t byte = data[i];
    if (byte == 0) {
      break;
    }
    if (byte < 0x80) {
      out.push_back(static_cast<char>(byte));
    } else {
      appendUtf8(out, kCp1251High[byte - 0x80]);
    }
  }
  return out;
}

std::string latin1ToUtf8(const uint8_t* data, size_t length) {
  std::string out;
  out.reserve(length * 2);
  if (data == nullptr) {
    return out;
  }
  for (size_t i = 0; i < length; ++i) {
    const uint8_t byte = data[i];
    if (byte == 0) {
      break;
    }
    appendUtf8(out, byte);
  }
  return out;
}

}  // namespace ipsb
