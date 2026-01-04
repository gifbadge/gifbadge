/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <time.h>
#include <stdint-gcc.h>
#include "portable_time.h"

int64_t get_millis(){
  struct timespec te;
  clock_gettime(CLOCK_BOOTTIME, &te);
  int64_t milliseconds = te.tv_sec*1000LL + (te.tv_nsec/1000/1000); // calculate milliseconds
  // printf("milliseconds: %lld\n", milliseconds);
  return milliseconds;
}