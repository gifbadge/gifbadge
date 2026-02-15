/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#ifndef BENCHMARK_SLEEP_H_
#define BENCHMARK_SLEEP_H_

namespace benchmark {
const int kNumMillisPerSecond = 1000;
const int kNumMicrosPerMilli = 1000;
const int kNumMicrosPerSecond = kNumMillisPerSecond * 1000;
const int kNumNanosPerMicro = 1000;
const int kNumNanosPerSecond = kNumNanosPerMicro * kNumMicrosPerSecond;

void SleepForMilliseconds(int milliseconds);
void SleepForSeconds(double seconds);
}  // end namespace benchmark

#endif  // BENCHMARK_SLEEP_H_
