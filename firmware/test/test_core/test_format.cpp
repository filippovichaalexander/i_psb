#include <unity.h>

#include "ipsb/format.h"

void test_format_time() {
    TEST_ASSERT_EQUAL_STRING("0:00", ipsb::formatTime(0).c_str());
    TEST_ASSERT_EQUAL_STRING("0:59", ipsb::formatTime(59).c_str());
    TEST_ASSERT_EQUAL_STRING("1:00", ipsb::formatTime(60).c_str());
    TEST_ASSERT_EQUAL_STRING("1:01:01", ipsb::formatTime(3661).c_str());
}

void test_format_bytes_and_percent() {
    TEST_ASSERT_EQUAL_STRING("500 B", ipsb::formatBytes(500).c_str());
    TEST_ASSERT_EQUAL_STRING("1.0 KB", ipsb::formatBytes(1024).c_str());
    TEST_ASSERT_EQUAL_STRING("1.0 MB", ipsb::formatBytes(1024 * 1024).c_str());
    TEST_ASSERT_EQUAL_STRING("42%", ipsb::formatPercent(42).c_str());
}
