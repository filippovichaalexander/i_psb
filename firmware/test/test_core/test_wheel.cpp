#include <unity.h>

#include "ipsb/wheel.h"

void test_wheel_clockwise() {
    ipsb::WheelDecoder wheel(8);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, wheel.feed(0, 0));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, wheel.feed(1u << 0, 10));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, wheel.feed(1u << 1, 20));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, wheel.feed(1u << 2, 30));
}

void test_wheel_counter_clockwise() {
    ipsb::WheelDecoder wheel(8);
    wheel.feed(1u << 2, 10);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, wheel.feed(1u << 1, 20));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, wheel.feed(1u << 0, 30));
}

void test_wheel_wrap() {
    ipsb::WheelDecoder wheel(8);
    wheel.feed(1u << 7, 10);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, wheel.feed(1u << 0, 20));
}

void test_wheel_big_jump_ignored() {
    ipsb::WheelDecoder wheel(8);
    wheel.feed(1u << 0, 10);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, wheel.feed(1u << 4, 20));
}

void test_wheel_multikey_centroid() {
    ipsb::WheelDecoder wheel(8);
    wheel.feed(0b00000011, 10);  // sectors 0+1 -> centre 0.5
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, wheel.feed(0b00000110, 20));  // 1+2 -> 1.5
}

void test_wheel_reset_after_gap() {
    ipsb::WheelDecoder wheel(8);
    wheel.feed(1u << 0, 1000);
    wheel.feed(0, 1100);
    wheel.feed(0, 1200);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, wheel.feed(1u << 3, 1300));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, wheel.feed(1u << 4, 1400));
}
