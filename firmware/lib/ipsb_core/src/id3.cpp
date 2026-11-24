#include "ipsb/id3.h"

#include "ipsb/utf8.h"

#include <cstring>

namespace ipsb {
namespace {

uint32_t syncsafeToUint32(const uint8_t* p) {
    return (static_cast<uint32_t>(p[0] & 0x7F) << 21) | (static_cast<uint32_t>(p[1] & 0x7F) << 14) |
           (static_cast<uint32_t>(p[2] & 0x7F) << 7) | static_cast<uint32_t>(p[3] & 0x7F);
}

uint32_t bigEndianToUint32(const uint8_t* p) {
    return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
}

std::string decodeUtf16(const uint8_t* data, size_t length, bool littleEndian) {
    std::string out;
    out.reserve(length);
    size_t i = 0;
    while (i + 1 < length) {
        const uint16_t first = littleEndian ? static_cast<uint16_t>(data[i] | (data[i + 1] << 8))
                                            : static_cast<uint16_t>((data[i] << 8) | data[i + 1]);
        i += 2;
        if (first == 0) {
            break;
        }
        uint32_t cp = first;
        if (first >= 0xD800 && first <= 0xDBFF && i + 1 < length) {
            const uint16_t second = littleEndian
                                        ? static_cast<uint16_t>(data[i] | (data[i + 1] << 8))
                                        : static_cast<uint16_t>((data[i] << 8) | data[i + 1]);
            if (second >= 0xDC00 && second <= 0xDFFF) {
                i += 2;
                cp = 0x10000 + ((static_cast<uint32_t>(first - 0xD800) << 10) |
                                static_cast<uint32_t>(second - 0xDC00));
            }
        }
        // Encode manually to keep the core dependency-free.
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
    return out;
}

std::string decodeText(uint8_t encoding, const uint8_t* data, size_t length) {
    if (data == nullptr || length == 0) {
        return {};
    }

    std::string value;
    switch (encoding) {
        case 0:
            // ID3 says ISO-8859-1, but Russian taggers commonly write CP1251 here.
            value = cp1251ToUtf8(data, length);
            break;
        case 1: {
            bool little = true;
            size_t offset = 0;
            if (length >= 2 && data[0] == 0xFF && data[1] == 0xFE) {
                offset = 2;
                little = true;
            } else if (length >= 2 && data[0] == 0xFE && data[1] == 0xFF) {
                offset = 2;
                little = false;
            }
            value = decodeUtf16(data + offset, length - offset, little);
            break;
        }
        case 2:
            value = decodeUtf16(data, length, false);
            break;
        case 3:
        default:
            value.assign(reinterpret_cast<const char*>(data), length);
            break;
    }

    while (!value.empty() && (value.back() == '\0' || value.back() == ' ' || value.back() == '\r' ||
                              value.back() == '\n')) {
        value.pop_back();
    }
    return value;
}

bool isFrameIdChar(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

}  // namespace

Id3Tags parseId3v2(const uint8_t* data, size_t length) {
    Id3Tags tags;
    if (data == nullptr || length < 10) {
        return tags;
    }
    if (data[0] != 'I' || data[1] != 'D' || data[2] != '3') {
        return tags;
    }

    const uint8_t version = data[3];
    if (version != 3 && version != 4) {
        return tags;
    }

    const uint32_t declaredSize = syncsafeToUint32(data + 6);
    const size_t end = (declaredSize + 10 < length) ? declaredSize + 10 : length;
    size_t pos = 10;

    while (pos + 10 <= end) {
        const char* id = reinterpret_cast<const char*>(data + pos);
        if (!isFrameIdChar(id[0]) || !isFrameIdChar(id[1]) || !isFrameIdChar(id[2]) ||
            !isFrameIdChar(id[3])) {
            break;
        }

        const uint32_t frameSize =
            (version == 4) ? syncsafeToUint32(data + pos + 4) : bigEndianToUint32(data + pos + 4);
        if (frameSize == 0 || pos + 10 + frameSize > end) {
            break;
        }

        const uint8_t* payload = data + pos + 10;
        const size_t payloadSize = frameSize;
        const uint8_t encoding = payload[0];
        const std::string value = decodeText(encoding, payload + 1, payloadSize - 1);

        if (std::strncmp(id, "TIT2", 4) == 0) {
            tags.title = value;
        } else if (std::strncmp(id, "TPE1", 4) == 0) {
            tags.artist = value;
        } else if (std::strncmp(id, "TALB", 4) == 0) {
            tags.album = value;
        }

        pos += 10 + frameSize;
    }

    tags.found = !tags.empty();
    return tags;
}

}  // namespace ipsb
