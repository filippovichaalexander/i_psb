#include <unity.h>

#include "ipsb/volume.h"

void test_volume_startup() {
  TEST_ASSERT_EQUAL_UINT8(4, ipsb::kVolumeStartupStep);
  TEST_ASSERT_EQUAL_UINT8(19, ipsb::volumePercent(ipsb::kVolumeStartupStep));
}

void test_volume_clamp() {
  TEST_ASSERT_EQUAL_UINT8(ipsb::kVolumeSafeMaxStep, ipsb::clampVolume(25));
  TEST_ASSERT_EQUAL_UINT8(ipsb::kVolumeSafeMaxStep, ipsb::clampVolume(18));
  TEST_ASSERT_EQUAL_UINT8(10, ipsb::clampVolume(10));
  TEST_ASSERT_EQUAL_UINT8(21, ipsb::clampVolume(25, 21));
}
