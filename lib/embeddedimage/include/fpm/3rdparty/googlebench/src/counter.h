/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "benchmark/benchmark.h"

namespace benchmark {

// these counter-related functions are hidden to reduce API surface.
namespace internal {
void Finish(UserCounters *l, double time, double num_threads);
void Increment(UserCounters *l, UserCounters const& r);
bool SameNames(UserCounters const& l, UserCounters const& r);
} // end namespace internal

} //end namespace benchmark
