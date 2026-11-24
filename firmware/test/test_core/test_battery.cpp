#include <unity.h>

#include "ipsb/battery.h"

void test_battery_curve_low() {
    TEST_ASSERT_EQUAL_UINT8(0, ipsb::voltageToPercent(3250));
    TEST_ASSERT_EQUAL_UINT8(0, ipsb::voltageToPercent(3000));
}

void test_battery_curve_full() {
    TEST_ASSERT_EQUAL_UINT8(100, ipsb::voltageToPercent(4180));
    TEST_ASSERT_EQUAL_UINT8(100, ipsb::voltageToPercent(4300));
}

void test_battery_curve_mid() {
    const uint8_t pct = ipsb::voltageToPercent(3700);
    TEST_ASSERT_TRUE(pct >= 35 && pct <= 55);
    TEST_ASSERT_TRUE(ipsb::voltageToPercent(3900) > pct);
}

void test_battery_curve_clamps() {
    TEST_ASSERT_EQUAL_UINT8(0, ipsb::voltageToPercent(0));
    TEST_ASSERT_EQUAL_UINT8(100, ipsb::voltageToPercent(65535));
}

void test_battery_ema() {
    ipsb::EmaFilter filter(0.2f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, filter.update(0.0f));
    const float next = filter.update(100.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 20.0f, next);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 20.0f, filter.value());
}

void test_battery_divider() {
    TEST_ASSERT_EQUAL_UINT16(4200, ipsb::adcMillivoltsToBatteryMillivolts(2100));
    TEST_ASSERT_EQUAL_UINT16(3000, ipsb::adcMillivoltsToBatteryMillivolts(1500));
    TEST_ASSERT_EQUAL_UINT16(0, ipsb::adcMillivoltsToBatteryMillivolts(2100, 100, 0));
}
