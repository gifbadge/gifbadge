/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <malloc.h>
#include "unity.h"
#include "font_render.h"

void test_render_font(void) {
  void *buffer = malloc(480 * 480 * 2);;
  render_text_centered(480, 480, 10, "No Image", buffer);
  free(buffer);
}

void setUp(void) {
}

void tearDown(void) {
  // clean stuff up here
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_render_font);

  return UNITY_END();
}