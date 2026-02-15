/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#ifndef STATISTICS_H_
#define STATISTICS_H_

#include <vector>

#include "benchmark/benchmark.h"

namespace benchmark {

// Return a vector containing the mean, median and standard devation information
// (and any user-specified info) for the specified list of reports. If 'reports'
// contains less than two non-errored runs an empty vector is returned
std::vector<BenchmarkReporter::Run> ComputeStats(
    const std::vector<BenchmarkReporter::Run>& reports);

double StatisticsMean(const std::vector<double>& v);
double StatisticsMedian(const std::vector<double>& v);
double StatisticsStdDev(const std::vector<double>& v);

}  // end namespace benchmark

#endif  // STATISTICS_H_
