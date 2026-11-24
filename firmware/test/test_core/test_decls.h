#pragma once

void test_battery_curve_low();
void test_battery_curve_full();
void test_battery_curve_mid();
void test_battery_curve_clamps();
void test_battery_ema();
void test_battery_divider();

void test_volume_startup();
void test_volume_clamp();

void test_playlist_sequential();
void test_playlist_repeat_all();
void test_playlist_repeat_one();
void test_playlist_previous_boundary();
void test_playlist_shuffle();

void test_wheel_clockwise();
void test_wheel_counter_clockwise();
void test_wheel_wrap();
void test_wheel_big_jump_ignored();
void test_wheel_multikey_centroid();
void test_wheel_reset_after_gap();

void test_utf8_length();
void test_utf8_truncate_boundary();
void test_utf8_cp1251();
void test_utf8_ellipsize();

void test_id3_utf16_title();
void test_id3_utf8_artist();
void test_id3_cp1251_album();
void test_id3_garbage();

void test_format_time();
void test_format_bytes_and_percent();
