#include <cstring>
#include <string>

#include <unity.h>

#include "ipsb/utf8.h"

void test_utf8_length() {
    TEST_ASSERT_EQUAL_size_t(6, ipsb::utf8Length("Привет"));
    TEST_ASSERT_EQUAL_size_t(0, ipsb::utf8Length(""));
    TEST_ASSERT_EQUAL_size_t(3, ipsb::utf8Length("abc"));
}

void test_utf8_truncate_boundary() {
    // "AП" = 41 D0 9F: a 2-byte limit must not split the Cyrillic letter.
    TEST_ASSERT_EQUAL_size_t(1, ipsb::utf8SafeTruncate("AП", 2));
    // "Привет": first two letters occupy 4 bytes.
    TEST_ASSERT_EQUAL_size_t(4, ipsb::utf8SafeTruncate("Привет", 5));
    TEST_ASSERT_EQUAL_size_t(12, ipsb::utf8SafeTruncate("Привет", 100));
}

void test_utf8_cp1251() {
    const uint8_t cp1251[] = {0xCF, 0xF0, 0xE8, 0xE2, 0xE5, 0xF2};  // "Привет"
    const std::string utf8 = ipsb::cp1251ToUtf8(cp1251, sizeof(cp1251));
    TEST_ASSERT_EQUAL_STRING("Привет", utf8.c_str());
}

void test_utf8_ellipsize() {
    TEST_ASSERT_EQUAL_STRING("Привет", ipsb::utf8Ellipsize("Привет", 32).c_str());
    const std::string cut = ipsb::utf8Ellipsize("Привет", 8);
    TEST_ASSERT_EQUAL_STRING("Пр...", cut.c_str());
    TEST_ASSERT_EQUAL_STRING("...", ipsb::utf8Ellipsize("Привет", 3).c_str());
}
