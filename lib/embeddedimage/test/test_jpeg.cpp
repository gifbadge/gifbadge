/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <sys/stat.h>
#include "config.h"
#include "unity.h"
#include "jpeg.h"
#include "test_jpeg.h"
#include "testdata/480x480.h"
#include "testdata/240x240on480x480jpeg.h"
#include "testdata/437x437on480x480.h"

#include <filesystem>



void test_jpeg_480_open() {
  auto *frame = static_cast<uint8_t *>(malloc(480*480*2));
  memset(frame, 0, 480*480*2);
  auto img = new image::JPEG( {480, 480}, (file_path/std::filesystem::path("480x480.jpeg")).c_str());
  // printf("%s\n", (file_path/std::filesystem::path("480x480.jpeg")).c_str());
  TEST_ASSERT_EQUAL(0, img->Open(nullptr));
  TEST_ASSERT_TRUE(image::screenResolution(480, 480) == img->Size());
  TEST_ASSERT_EQUAL(image::frameStatus::END, img->GetFrame(frame, 0, 0, 480).first);
  TEST_ASSERT_EQUAL(0, memcmp(png_480.pixel_data, frame, sizeof(480*480*2)));
  free(frame);
  delete img;
}

void test_jpeg_480_redraw() {
  auto *frame = static_cast<uint8_t *>(malloc(480*480*2));
  memset(frame, 0, 480*480*2);
  auto img = new image::JPEG({480, 480}, (file_path/std::filesystem::path("480x480.jpeg")).c_str());

  TEST_ASSERT_EQUAL(0, img->Open(nullptr));
  TEST_ASSERT_EQUAL(image::frameStatus::END, img->GetFrame(frame, 0, 0, 480).first);
  TEST_ASSERT_EQUAL(0, memcmp(png_480.pixel_data, frame, sizeof(480*480*2)));
  memset(frame, 0, 480*480*2);
  TEST_ASSERT_EQUAL(image::frameStatus::END, img->GetFrame(frame, 0, 0, 480).first);
  TEST_ASSERT_EQUAL(0, memcmp(png_480.pixel_data, frame, sizeof(480*480*2)));
  free(frame);
  delete img;
}

void test_jpeg_480_progressive_open() {
  auto img = new image::JPEG({480, 480}, (file_path/std::filesystem::path("480x480_progressive.jpeg")).c_str());
  TEST_ASSERT_EQUAL(1, img->Open(nullptr));
  printf("%s\n", img->GetLastError());
  TEST_ASSERT_EQUAL(0, strcmp("JPEG_PROGRESSIVE_NOT_SUPPORTED", img->GetLastError()));
  delete img;
}

void test_jpeg_240_on_480() {
  //This test seems to need its own result to compare against due to aliasing or something in JPEGDEC
  auto *frame = static_cast<uint8_t *>(malloc(480*480*2));
  memset(frame, 0, 480*480*2);
  auto img = new image::JPEG({480, 480}, (file_path/std::filesystem::path("240x240.jpeg")).c_str());

  TEST_ASSERT_EQUAL(0, img->Open(nullptr));
  TEST_ASSERT_EQUAL(image::frameStatus::END, img->GetFrame(frame, 0, 0, 480).first);
  TEST_ASSERT_EQUAL(0, memcmp(jpeg240on480.pixel_data, frame, sizeof(480*480*2)));
  free(frame);
  delete img;
}

void test_jpeg_437_on_480() {
  auto *frame = static_cast<uint8_t *>(malloc(480*480*2));
  memset(frame, 0, 480*480*2);
  auto img = new image::JPEG({480, 480}, (file_path/std::filesystem::path("437x437.jpeg")).c_str());

  TEST_ASSERT_EQUAL(0, img->Open(nullptr));
  TEST_ASSERT_EQUAL(image::frameStatus::END, img->GetFrame(frame, 0, 0, 480).first);
  printf("%s\n", img->GetLastError());
  FILE *out = fopen("test.data", "wb");
  fwrite(frame, sizeof(uint8_t), 480*480*2, out);
  fclose(out);
  TEST_ASSERT_EQUAL(0, memcmp(png437on480.pixel_data, frame, sizeof(480*480*2)));
  free(frame);
  delete img;
}


