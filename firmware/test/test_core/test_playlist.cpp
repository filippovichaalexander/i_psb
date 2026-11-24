#include <unity.h>

#include "ipsb/playlist.h"

void test_playlist_sequential() {
  ipsb::PlaylistCursor cursor(3, ipsb::RepeatMode::Off);
  TEST_ASSERT_EQUAL_size_t(0, cursor.current());
  TEST_ASSERT_TRUE(cursor.next());
  TEST_ASSERT_EQUAL_size_t(1, cursor.current());
  TEST_ASSERT_TRUE(cursor.next());
  TEST_ASSERT_EQUAL_size_t(2, cursor.current());
  TEST_ASSERT_FALSE(cursor.next());
  TEST_ASSERT_EQUAL_size_t(2, cursor.current());
}

void test_playlist_repeat_all() {
  ipsb::PlaylistCursor cursor(3, ipsb::RepeatMode::All);
  cursor.setCurrent(2);
  TEST_ASSERT_TRUE(cursor.next());
  TEST_ASSERT_EQUAL_size_t(0, cursor.current());
}

void test_playlist_repeat_one() {
  ipsb::PlaylistCursor cursor(3, ipsb::RepeatMode::One);
  cursor.setCurrent(1);
  TEST_ASSERT_TRUE(cursor.next());
  TEST_ASSERT_EQUAL_size_t(1, cursor.current());
}

void test_playlist_previous_boundary() {
  ipsb::PlaylistCursor cursor(3, ipsb::RepeatMode::Off);
  TEST_ASSERT_FALSE(cursor.previous());
  TEST_ASSERT_EQUAL_size_t(0, cursor.current());

  cursor.setRepeatMode(ipsb::RepeatMode::All);
  TEST_ASSERT_TRUE(cursor.previous());
  TEST_ASSERT_EQUAL_size_t(2, cursor.current());
}

void test_playlist_shuffle() {
  ipsb::PlaylistCursor cursor(5, ipsb::RepeatMode::Off, true, 0xDEADBEEF);
  TEST_ASSERT_EQUAL_size_t(0, cursor.current());
  TEST_ASSERT_TRUE(cursor.next());
  const size_t first = cursor.current();
  TEST_ASSERT_TRUE(first != 0);
  TEST_ASSERT_TRUE(cursor.next());
  TEST_ASSERT_TRUE(cursor.current() != first);
}
