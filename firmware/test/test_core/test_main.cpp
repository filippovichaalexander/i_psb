#include <unity.h>

#include "test_decls.h"

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();

    RUN_TEST(test_battery_curve_low);
    RUN_TEST(test_battery_curve_full);
    RUN_TEST(test_battery_curve_mid);
    RUN_TEST(test_battery_curve_clamps);
    RUN_TEST(test_battery_ema);
    RUN_TEST(test_battery_divider);

    RUN_TEST(test_volume_startup);
    RUN_TEST(test_volume_clamp);

    RUN_TEST(test_playlist_sequential);
    RUN_TEST(test_playlist_repeat_all);
    RUN_TEST(test_playlist_repeat_one);
    RUN_TEST(test_playlist_previous_boundary);
    RUN_TEST(test_playlist_shuffle);

    RUN_TEST(test_wheel_clockwise);
    RUN_TEST(test_wheel_counter_clockwise);
    RUN_TEST(test_wheel_wrap);
    RUN_TEST(test_wheel_big_jump_ignored);
    RUN_TEST(test_wheel_multikey_centroid);
    RUN_TEST(test_wheel_reset_after_gap);

    RUN_TEST(test_utf8_length);
    RUN_TEST(test_utf8_truncate_boundary);
    RUN_TEST(test_utf8_cp1251);
    RUN_TEST(test_utf8_ellipsize);

    RUN_TEST(test_id3_utf16_title);
    RUN_TEST(test_id3_utf8_artist);
    RUN_TEST(test_id3_cp1251_album);
    RUN_TEST(test_id3_garbage);

    RUN_TEST(test_format_time);
    RUN_TEST(test_format_bytes_and_percent);

    return UNITY_END();
}
