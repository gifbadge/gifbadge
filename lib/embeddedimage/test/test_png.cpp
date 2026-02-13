/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <sys/stat.h>
#include "config.h"
#include "unity.h"
#include "png.h"
#include "test_png.h"
#include "testdata/480x480.h"
#include "testdata/240x240on480x480.h"
#include "testdata/437x437on480x480.h"
#include "testdata/480x480png_resized_240x240.h"
#include "testdata/960x480_resized_480x360.h"

#include <filesystem>

void test_png_480_open() {
  auto *frame = static_cast<uint8_t *>(malloc(480*480*2));
  memset(frame, 0, 480*480*2);
  auto img = new image::PNGImage({480, 480}, (file_path/std::filesystem::path("480x480.png")).c_str());

  TEST_ASSERT_EQUAL(0, img->Open(nullptr));
  TEST_ASSERT_TRUE(image::screenResolution(480, 480) == img->Size());
  TEST_ASSERT_EQUAL(image::frameStatus::END, img->GetFrame(frame, 0, 0, 480).first);
  TEST_ASSERT_EQUAL(0, memcmp(png_480.pixel_data, frame, sizeof(480*480*2)));
  free(frame);
  delete img;
}

void test_png_480_redraw() {
  auto *frame = static_cast<uint8_t *>(malloc(480*480*2));
  memset(frame, 0, 480*480*2);
  auto img = new image::PNGImage({480, 480}, (file_path/std::filesystem::path("480x480.png")).c_str());

  TEST_ASSERT_EQUAL(0, img->Open( nullptr));
  TEST_ASSERT_EQUAL(image::frameStatus::END, img->GetFrame(frame, 0, 0, 480).first);
  TEST_ASSERT_EQUAL(0, memcmp(png_480.pixel_data, frame, sizeof(480*480*2)));
  memset(frame, 0, 480*480*2);
  TEST_ASSERT_EQUAL(image::frameStatus::END, img->GetFrame(frame, 0, 0, 480).first);
  TEST_ASSERT_EQUAL(0, memcmp(png_480.pixel_data, frame, sizeof(480*480*2)));
  free(frame);
  delete img;
}

void test_png_480i_open() {
  auto img = new image::PNGImage({480, 480}, (file_path/std::filesystem::path("480x480_interlaced.png")).c_str());

  TEST_ASSERT_EQUAL(5, img->Open( nullptr));
  delete img;
}

void test_png_240_on_480() {
  auto *frame = static_cast<uint8_t *>(malloc(480*480*2));
  memset(frame, 0, 480*480*2);
  auto img = new image::PNGImage({480, 480}, (file_path/std::filesystem::path("240x240.png")).c_str());

  TEST_ASSERT_EQUAL(0, img->Open( nullptr));
  TEST_ASSERT_EQUAL(image::frameStatus::END, img->GetFrame(frame, 0, 0, 480).first);
  TEST_ASSERT_EQUAL(0, memcmp(png240on480.pixel_data, frame, sizeof(480*480*2)));
  free(frame);
  delete img;
}

void test_png_437_on_480() {
  auto *frame = static_cast<uint8_t *>(malloc(480*480*2));
  memset(frame, 0, 480*480*2);
  auto img = new image::PNGImage({480, 480}, (file_path/std::filesystem::path("437x437.png")).c_str());

  TEST_ASSERT_EQUAL(0, img->Open( nullptr));
  TEST_ASSERT_EQUAL(image::frameStatus::END, img->GetFrame(frame, 0, 0, 480).first);
  TEST_ASSERT_EQUAL(0, memcmp(png437on480.pixel_data, frame, sizeof(480*480*2)));
  free(frame);
  delete img;
}

void test_png_480_on_240() {
  auto *frame = static_cast<uint8_t *>(malloc(240*240*2));
  memset(frame, 0, 240*240*2);
  auto img = new image::PNGImage({240, 240}, (file_path/std::filesystem::path("480x480.png")).c_str());

  auto *buffer = static_cast<uint8_t *>(malloc(480 * 480 + 0x6100));
  TEST_ASSERT_EQUAL(0, img->Open( buffer));
  TEST_ASSERT_EQUAL(0, img->resize(frame, 0, 0, 240, 240));
  TEST_ASSERT_EQUAL(0, memcmp(png480x480resized240x240.pixel_data, frame, sizeof(240*240*2)));
  free(buffer);
  free(frame);
  delete img;
}

void test_png_720_on_240() {
  auto *frame = static_cast<uint8_t *>(malloc(240*240*2));
  memset(frame, 0, 240*240*2);
  auto img = new image::PNGImage({240, 240}, (file_path/std::filesystem::path("720x720.png")).c_str());

  auto *buffer = static_cast<uint8_t *>(malloc(480 * 480 + 0x6100));
  TEST_ASSERT_EQUAL(0, img->Open( buffer));
  TEST_ASSERT_EQUAL(0, img->resize(frame, 0, 0, 240, 240));
  TEST_ASSERT_EQUAL(0, memcmp(png480x480resized240x240.pixel_data, frame, sizeof(240*240*2)));
  free(buffer);
  free(frame);
  delete img;
}

void test_png_960x720_on_480() {
  auto *frame = static_cast<uint8_t *>(malloc(480*480*2));
  memset(frame, 0, 480*480*2);
  auto img = new image::PNGImage({480, 480}, (file_path/std::filesystem::path("960x720.png")).c_str());

  auto *buffer = static_cast<uint8_t *>(malloc(480 * 480 + 0x6100));
  TEST_ASSERT_EQUAL(0, img->Open( buffer));
  TEST_ASSERT_EQUAL(0, img->resize(frame, 0, 0, 480, 480));
  TEST_ASSERT_EQUAL(0, memcmp(png960x720resized480x360.pixel_data, frame, sizeof(480*480*2)));
  free(buffer);
  free(frame);
  delete img;
}