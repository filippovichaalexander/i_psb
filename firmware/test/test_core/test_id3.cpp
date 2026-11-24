#include <cstdint>
#include <vector>

#include <unity.h>

#include "ipsb/id3.h"

namespace {

void appendBigEndian32(std::vector<uint8_t>& out, uint32_t value) {
  out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
  out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
  out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
  out.push_back(static_cast<uint8_t>(value & 0xFF));
}

uint32_t syncsafe(uint32_t value) {
  return ((value & 0x7F) << 21) | (((value >> 7) & 0x7F) << 14) |
         (((value >> 14) & 0x7F) << 7) | ((value >> 21) & 0x7F);
}

void appendFrame(std::vector<uint8_t>& frames, const char* id,
                 const std::vector<uint8_t>& payload) {
  frames.insert(frames.end(), id, id + 4);
  appendBigEndian32(frames, static_cast<uint32_t>(payload.size()));
  frames.push_back(0);
  frames.push_back(0);
  frames.insert(frames.end(), payload.begin(), payload.end());
}

std::vector<uint8_t> makeTag(const std::vector<uint8_t>& frames) {
  std::vector<uint8_t> tag = {'I', 'D', '3', 3, 0, 0};
  const uint32_t size = syncsafe(static_cast<uint32_t>(frames.size()));
  tag.push_back(static_cast<uint8_t>((size >> 21) & 0x7F));
  tag.push_back(static_cast<uint8_t>((size >> 14) & 0x7F));
  tag.push_back(static_cast<uint8_t>((size >> 7) & 0x7F));
  tag.push_back(static_cast<uint8_t>(size & 0x7F));
  tag.insert(tag.end(), frames.begin(), frames.end());
  return tag;
}

}  // namespace

void test_id3_utf16_title() {
  // TIT2: encoding 1 (UTF-16LE with BOM), "Test"
  std::vector<uint8_t> payload = {1, 0xFF, 0xFE, 'T', 0, 'e', 0, 's', 0, 't', 0};
  std::vector<uint8_t> frames;
  appendFrame(frames, "TIT2", payload);
  const std::vector<uint8_t> tag = makeTag(frames);
  const ipsb::Id3Tags tags = ipsb::parseId3v2(tag.data(), tag.size());
  TEST_ASSERT_TRUE(tags.found);
  TEST_ASSERT_EQUAL_STRING("Test", tags.title.c_str());
}

void test_id3_utf8_artist() {
  std::vector<uint8_t> payload = {3, 'A', 'r', 't', 'i', 's', 't'};
  std::vector<uint8_t> frames;
  appendFrame(frames, "TPE1", payload);
  const std::vector<uint8_t> tag = makeTag(frames);
  const ipsb::Id3Tags tags = ipsb::parseId3v2(tag.data(), tag.size());
  TEST_ASSERT_EQUAL_STRING("Artist", tags.artist.c_str());
}

void test_id3_cp1251_album() {
  // TALB: encoding 0, CP1251 "Альбом"
  std::vector<uint8_t> payload = {0, 0xC0, 0xEB, 0xFC, 0xE1, 0xEE, 0xEC};
  std::vector<uint8_t> frames;
  appendFrame(frames, "TALB", payload);
  const std::vector<uint8_t> tag = makeTag(frames);
  const ipsb::Id3Tags tags = ipsb::parseId3v2(tag.data(), tag.size());
  TEST_ASSERT_EQUAL_STRING("Альбом", tags.album.c_str());
}

void test_id3_garbage() {
  TEST_ASSERT_TRUE(ipsb::parseId3v2(nullptr, 0).empty());
  const uint8_t garbage[] = {0x00, 0x01, 0x02, 0x03, 0x04};
  TEST_ASSERT_TRUE(ipsb::parseId3v2(garbage, sizeof(garbage)).empty());

  // Header claims a frame larger than the buffer: must not read out of bounds.
  const uint8_t truncated[] = {'I', 'D', '3', 3, 0, 0, 0, 0, 0, 20,
                               'T', 'I', 'T', '2', 0, 0, 0, 50, 0, 0, 3, 'x'};
  TEST_ASSERT_TRUE(ipsb::parseId3v2(truncated, sizeof(truncated)).empty());
}
