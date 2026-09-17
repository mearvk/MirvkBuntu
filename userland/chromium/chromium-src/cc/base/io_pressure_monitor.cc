// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// GALACTIC CHERRY MARVELL EDITION 98 — I/O Pressure Monitor Implementation

#include "cc/base/io_pressure_monitor.h"

#include <cstdio>
#include <cstring>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/time/time.h"
#include "build/build_config.h"

namespace cc {

namespace {

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS) || BUILDFLAG(IS_ANDROID)
constexpr char kDefaultPSIPath[] = "/proc/pressure/io";
#endif

}  // namespace

// static
IOPressureMonitor& IOPressureMonitor::GetInstance() {
  static base::NoDestructor<IOPressureMonitor> instance;
  return *instance;
}

IOPressureMonitor::IOPressureMonitor() {
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS) || BUILDFLAG(IS_ANDROID)
  pressure_path_ = base::FilePath(kDefaultPSIPath);
  psi_available_ = base::PathExists(pressure_path_);
  if (psi_available_) {
    // Initial read to populate cached values.
    ReadPressureFile();
  }
#else
  // PSI is Linux-only. On other platforms, always report no pressure.
  psi_available_ = false;
#endif
}

bool IOPressureMonitor::IsUnderIOPressure() const {
  MaybeRefresh();
  return under_pressure_.load(std::memory_order_relaxed);
}

float IOPressureMonitor::GetSomeAvg10() const {
  MaybeRefresh();
  return some_avg10_.load(std::memory_order_relaxed);
}

float IOPressureMonitor::GetFullAvg10() const {
  MaybeRefresh();
  return full_avg10_.load(std::memory_order_relaxed);
}

void IOPressureMonitor::Refresh() {
  if (!psi_available_)
    return;
  ReadPressureFile();
}

void IOPressureMonitor::MaybeRefresh() const {
  if (!psi_available_)
    return;

  const int64_t now_us = base::TimeTicks::Now().since_origin().InMicroseconds();
  const int64_t last_us = last_refresh_us_.load(std::memory_order_relaxed);

  if (now_us - last_us < kIOPressurePollInterval.InMicroseconds())
    return;

  // Perform refresh. Race-safe: worst case we read slightly stale data.
  const_cast<IOPressureMonitor*>(this)->ReadPressureFile();
}

bool IOPressureMonitor::ReadPressureFile() {
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS) || BUILDFLAG(IS_ANDROID)
  // Read /proc/pressure/io directly. This is a procfs file and reads are
  // non-blocking and return immediately with current kernel state.
  // Format:
  //   some avg10=0.00 avg60=0.00 avg300=0.00 total=0
  //   full avg10=0.00 avg60=0.00 avg300=0.00 total=0
  char buf[256];
  int fd = open(pressure_path_.value().c_str(), O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    return false;

  ssize_t bytes_read = read(fd, buf, sizeof(buf) - 1);
  close(fd);

  if (bytes_read <= 0)
    return false;

  buf[bytes_read] = '\0';

  // Parse "some" line.
  float some_avg10 = 0.0f;
  float full_avg10 = 0.0f;

  const char* some_line = strstr(buf, "some ");
  if (some_line) {
    const char* avg10_str = strstr(some_line, "avg10=");
    if (avg10_str) {
      sscanf(avg10_str, "avg10=%f", &some_avg10);
    }
  }

  // Parse "full" line.
  const char* full_line = strstr(buf, "full ");
  if (full_line) {
    const char* avg10_str = strstr(full_line, "avg10=");
    if (avg10_str) {
      sscanf(avg10_str, "avg10=%f", &full_avg10);
    }
  }

  some_avg10_.store(some_avg10, std::memory_order_relaxed);
  full_avg10_.store(full_avg10, std::memory_order_relaxed);
  under_pressure_.store(
      some_avg10 > kIOPressureSomeThreshold ||
          full_avg10 > kIOPressureFullThreshold,
      std::memory_order_relaxed);
  last_refresh_us_.store(
      base::TimeTicks::Now().since_origin().InMicroseconds(),
      std::memory_order_relaxed);

  return true;
#else
  return false;
#endif
}

}  // namespace cc
