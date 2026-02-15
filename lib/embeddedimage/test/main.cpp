/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <unity.h>
#include "test_png.h"
#include "test_jpeg.h"
#include "FreeRTOS.h"
#include "task.h"

void vApplicationStackOverflowHook( TaskHandle_t pxTask,
                                    char * pcTaskName ) {
}
void vApplicationTickHook() {}
void vApplicationMallocFailedHook( void )
{}

void setUp(void) {
}

void tearDown(void) {
}

int main() {
  UNITY_BEGIN();

  RUN_TEST(test_png_480_open);
  RUN_TEST(test_png_480_redraw);
  RUN_TEST(test_png_480i_open);
  RUN_TEST(test_png_240_on_480);
  RUN_TEST(test_png_437_on_480);
  RUN_TEST(test_png_480_on_240);
  RUN_TEST(test_png_720_on_240);
  RUN_TEST(test_jpeg_480_open);
  RUN_TEST(test_jpeg_480_redraw);
  RUN_TEST(test_jpeg_480_progressive_open);
  RUN_TEST(test_jpeg_240_on_480);
  RUN_TEST(test_jpeg_437_on_480);
  RUN_TEST(test_jpeg_480_on_240);
  RUN_TEST(test_jpeg_720_on_240);
  RUN_TEST(test_png_960x720_on_480);
  return UNITY_END();
}