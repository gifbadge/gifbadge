/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <string.h>
#include <sys/stat.h>
#include "unity.h"
#include "directory.h"

static int reject_a(const char *path, const char *file) {
  if (strcmp(file, "a") == 0) {
    return 0;
  }
  return 1;
}

void test_Invalid_Directory(void) {
  DIR_SORTED dirp;
  TEST_ASSERT_EQUAL(0, opendir_sorted(&dirp, "/tmp/invalid", NULL));
  closedir_sorted(&dirp);
}

void test_Empty_Directory(void) {
  DIR_SORTED dirp;
  TEST_ASSERT_EQUAL(1, opendir_sorted(&dirp, "./test_data/test1", NULL));
  TEST_ASSERT_NULL(readdir_sorted(&dirp));
  closedir_sorted(&dirp);
}

void test_Single_File_Directory(void) {
  DIR_SORTED dirp;
  TEST_ASSERT_EQUAL(1, opendir_sorted(&dirp, "./test_data/test3", NULL));
  TEST_ASSERT_NOT_NULL(readdir_sorted(&dirp));
  closedir_sorted(&dirp);
}

void test_sorted_directory(void) {
  DIR_SORTED dirp;
  TEST_ASSERT_EQUAL(1, opendir_sorted(&dirp, "./test_data/test2", NULL));
  TEST_ASSERT_EQUAL_STRING("a", readdir_sorted(&dirp)->d_name);
  closedir_sorted(&dirp);
}

void test_sorted_validator(void) {
  DIR_SORTED dirp;
  TEST_ASSERT_EQUAL(1, opendir_sorted(&dirp, "./test_data/test2", reject_a));
  TEST_ASSERT_EQUAL_STRING("a.gif", readdir_sorted(&dirp)->d_name);
  closedir_sorted(&dirp);
}

void test_get_position(void) {
  DIR_SORTED dirp;
  TEST_ASSERT_EQUAL(1, opendir_sorted(&dirp, "./test_data/test2", NULL));
  TEST_ASSERT_EQUAL(3, directory_get_position(&dirp, "b"));
  closedir_sorted(&dirp);
}

void test_get_position_invalid_file(void) {
  DIR_SORTED dirp;
  TEST_ASSERT_EQUAL(1, opendir_sorted(&dirp, "./test_data/test2", NULL));
  TEST_ASSERT_EQUAL(-1, directory_get_position(&dirp, "z"));
  closedir_sorted(&dirp);
}

void test_get_next(void) {
  DIR_SORTED dirp;
  TEST_ASSERT_EQUAL(1, opendir_sorted(&dirp, "./test_data/test2", NULL));
  TEST_ASSERT_EQUAL(3, directory_get_position(&dirp, "b"));
  TEST_ASSERT_EQUAL_STRING("b.gif", directory_get_increment(&dirp, 3, 1));
  closedir_sorted(&dirp);
}

void test_get_previous(void) {
  DIR_SORTED dirp;
  TEST_ASSERT_EQUAL(1, opendir_sorted(&dirp, "./test_data/test2", NULL));
  TEST_ASSERT_EQUAL(3, directory_get_position(&dirp, "b"));
  TEST_ASSERT_EQUAL_STRING("a.png", directory_get_increment(&dirp, 3, -1));
  closedir_sorted(&dirp);
}

void test_get_increment_positive_wrap(void) {
  DIR_SORTED dirp;
  TEST_ASSERT_EQUAL(1, opendir_sorted(&dirp, "./test_data/test2", NULL));
  TEST_ASSERT_EQUAL(6, directory_get_position(&dirp, "c.jpg"));
  TEST_ASSERT_EQUAL_STRING("a", directory_get_increment(&dirp, 6, 1));
  closedir_sorted(&dirp);
}

void test_get_increment_negative_wrap(void) {
  DIR_SORTED dirp;
  TEST_ASSERT_EQUAL(1, opendir_sorted(&dirp, "./test_data/test2", NULL));
  TEST_ASSERT_EQUAL(0, directory_get_position(&dirp, "a"));
  TEST_ASSERT_EQUAL_STRING("c.jpg", directory_get_increment(&dirp, 0, -1));
  closedir_sorted(&dirp);
}

void test_get_increment_empty_positive(void) {
  DIR_SORTED dirp;
  TEST_ASSERT_EQUAL(1, opendir_sorted(&dirp, "./test_data/test1", NULL));
  TEST_ASSERT_NULL(directory_get_increment(&dirp, 0, 1));
  closedir_sorted(&dirp);
}

void test_get_increment_empty_negative(void) {
  DIR_SORTED dirp;
  TEST_ASSERT_EQUAL(1, opendir_sorted(&dirp, "./test_data/test1", NULL));
  TEST_ASSERT_NULL(directory_get_increment(&dirp, 0, -1));
  closedir_sorted(&dirp);
}

void test_get_increment_single_positive(void) {
  DIR_SORTED dirp;
  TEST_ASSERT_EQUAL(1, opendir_sorted(&dirp, "./test_data/test3", NULL));
  TEST_ASSERT_NOT_NULL(directory_get_increment(&dirp, 0, 1));
  closedir_sorted(&dirp);
}

void test_get_increment_single_negative(void) {
  DIR_SORTED dirp;
  TEST_ASSERT_EQUAL(1, opendir_sorted(&dirp, "./test_data/test3", NULL));
  TEST_ASSERT_NOT_NULL(directory_get_increment(&dirp, 0, -1));
  closedir_sorted(&dirp);
}

void test_compare_path_with_trailing_slash_not_equal(void) {
  char a[] = "/this/is/a/test/path/";
  char b[] = "/this/is/a/different/path/";
  TEST_ASSERT_NOT_EQUAL(0, compare_path(a, b));
  TEST_ASSERT_NOT_EQUAL(0, compare_path(b, a));
}

void test_compare_path_without_trailing_slash_not_equal(void) {
  char a[] = "/this/is/a/test/path";
  char b[] = "/this/is/a/different/path";
  TEST_ASSERT_NOT_EQUAL(0, compare_path(a, b));
  TEST_ASSERT_NOT_EQUAL(0, compare_path(b, a));

}

void test_compare_path_with_trailing_slash_equal(void) {
  char a[] = "/this/is/a/test/path/";
  char b[] = "/this/is/a/test/path/";
  TEST_ASSERT_EQUAL(0, compare_path(a, b));
}

void test_compare_path_without_trailing_slash_equal(void) {
  char a[] = "/this/is/a/test/path";
  char b[] = "/this/is/a/test/path";
  TEST_ASSERT_EQUAL(0, compare_path(a, b));
}

void test_compare_path_with_mixed_trailing_slash_equal(void) {
  char a[] = "/this/is/a/test/path";
  char b[] = "/this/is/a/test/path/";
  TEST_ASSERT_EQUAL(0, compare_path(a, b));
  TEST_ASSERT_EQUAL(0, compare_path(b, a));
}

void test_compare_path_with_mixed_trailing_slash_not_equal(void) {
  char a[] = "/this/is/a/test/path/";
  char b[] = "/this/is/a/different/path";
  TEST_ASSERT_NOT_EQUAL(0, compare_path(a, b));
  TEST_ASSERT_NOT_EQUAL(0, compare_path(b, a));

}

void test_is_directory(void) {
  TEST_ASSERT_TRUE(is_directory("./test_data/test2/a"));
  TEST_ASSERT_FALSE(is_directory("./test_data/test2/a.gif"));
}

void setUp(void) {
  mkdir("./test_data", 0700);

  mkdir("./test_data/test1", 0700);

  mkdir("./test_data/test2", 0700);
  mkdir("./test_data/test2/a", 0700);
  mkdir("./test_data/test2/b", 0700);
  mkdir("./test_data/test2/c", 0700);
  fclose(fopen("./test_data/test2/a.gif", "w"));
  fclose(fopen("./test_data/test2/a.png", "w"));
  fclose(fopen("./test_data/test2/b.gif", "w"));
  fclose(fopen("./test_data/test2/c.jpg", "w"));
  mkdir("./test_data/test3", 0700);
  fclose(fopen("./test_data/test3/a.gif", "w"));
}

void tearDown(void) {
  // clean stuff up here
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_Invalid_Directory);
  RUN_TEST(test_Empty_Directory);
  RUN_TEST(test_Single_File_Directory);
  RUN_TEST(test_sorted_directory);
  RUN_TEST(test_sorted_validator);
  RUN_TEST(test_get_position);
  RUN_TEST(test_get_position_invalid_file);
  RUN_TEST(test_get_next);
  RUN_TEST(test_get_previous);
  RUN_TEST(test_get_increment_positive_wrap);
  RUN_TEST(test_get_increment_negative_wrap);
  RUN_TEST(test_get_increment_empty_positive);
  RUN_TEST(test_get_increment_empty_negative);
  RUN_TEST(test_get_increment_single_positive);
  RUN_TEST(test_get_increment_single_negative);
  RUN_TEST(test_compare_path_with_trailing_slash_not_equal);
  RUN_TEST(test_compare_path_without_trailing_slash_not_equal);
  RUN_TEST(test_compare_path_with_trailing_slash_equal);
  RUN_TEST(test_compare_path_without_trailing_slash_equal);
  RUN_TEST(test_compare_path_with_mixed_trailing_slash_equal);
  RUN_TEST(test_compare_path_with_mixed_trailing_slash_not_equal);
  RUN_TEST(test_is_directory);

  return UNITY_END();
}