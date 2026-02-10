/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <sys/stat.h>
#include "unity.h"
#include "file.h"

void test_is_file() {
  TEST_ASSERT_TRUE(is_file("./test_data/a.gif"));
  TEST_ASSERT_FALSE(is_file("./test_data/"));
}

void test_extension() {
  std::array<const char *, 4> extensions = {".gif", ".jpg", ".jpeg", ".png"};
  std::span extensionSpan(extensions);
  TEST_ASSERT_TRUE(is_valid_extension("a.gif", extensionSpan));
  TEST_ASSERT_FALSE(is_valid_extension("a.bbb", extensionSpan));
  TEST_ASSERT_FALSE(is_valid_extension("a", extensionSpan));
}

void test_valid_image_file() {
  std::array<const char *, 4> extensions = {".gif", ".jpg", ".jpeg", ".png"};
  std::span extensionSpan(extensions);
  TEST_ASSERT_TRUE(valid_image_file("./test_data/a.gif", extensionSpan));
}

void test_is_not_hidden() {
  TEST_ASSERT_FALSE(is_not_hidden(".a.gif"));
  TEST_ASSERT_FALSE(is_not_hidden("~a.gif"));
  TEST_ASSERT_TRUE(is_not_hidden("a.gif"));
}

void setUp(void) {
  mkdir("./test_data", 0700);
  fclose(fopen("./test_data/a.gif", "w"));
}

void tearDown(void) {
  // clean stuff up here
}

int main() {
  UNITY_BEGIN();

  RUN_TEST(test_is_file);
  RUN_TEST(test_extension);
  RUN_TEST(test_valid_image_file);
  RUN_TEST(test_is_not_hidden);

  return UNITY_END();
}