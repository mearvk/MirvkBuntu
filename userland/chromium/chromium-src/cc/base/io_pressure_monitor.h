// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// GALACTIC CHERRY MARVELL EDITION 98 — I/O Pressure Monitor
//
// This module monitors system I/O pressure to allow the compositor and other
// latency-sensitive threads to make informed decisions about whether to block.
//
// PROBLEM: When the system is under heavy I/O pressure (e.g., git gc repacking
// a 40 GB repository, USB swap thrash, or large file indexing), blocking calls
// that normally complete in microseconds can take seconds. The compositor's
// blocking commit, tile raster, and inotify watchers are all affected.
//
// SOLUTION: Expose a lightweight I/O pressure query. Latency-sensitive code
// can check IsUnderIOPressure() before deciding to block indefinitely vs.
// using a timeout.
//
// On Linux, reads /proc/pressure/io (PSI) which provides:
//   some avg10=X.XX avg60=X.XX avg300=X.XX total=NNNNN
//   full avg10=X.XX avg60=X.XX avg300=X.XX total=NNNNN
//
// "some" means at least one task is stalled on I/O.
// "full" means ALL tasks are stalled on I/O (total system freeze).
//
// We consider the system under I/O pressure when:
//   - avg10 for "some" > 25.0 (25% of last 10s had I/O stalls), OR
//   - avg10 for "full" > 5.0 (5% of last 10s was a total I/O stall)

#ifndef CC_BASE_IO_PRESSURE_MONITOR_H_
#define CC_BASE_IO_PRESSURE_MONITOR_H_

#include <atomic>

#include "base/base_export.h"
#include "base/files/file_path.h"
#include "base/time/time.h"

namespace cc {

// Thresholds for considering the system under I/O pressure.
// These can be tuned per-system. Conservative defaults chosen to avoid
// false positives while catching genuine disk saturation.
inline constexpr float kIOPressureSomeThreshold = 25.0f;  // avg10 "some" > 25%
inline constexpr float kIOPressureFullThreshold = 5.0f;   // avg10 "full" > 5%

// How often to re-read /proc/pressure/io (avoid per-frame syscall overhead).
inline constexpr base::TimeDelta kIOPressurePollInterval =
    base::Milliseconds(500);

class BASE_EXPORT IOPressureMonitor {
 public:
  static IOPressureMonitor& GetInstance();

  // Returns true if the system is currently under I/O pressure.
  // Thread-safe. Cached result updated every kIOPressurePollInterval.
  bool IsUnderIOPressure() const;

  // Returns the current avg10 "some" value (0-100).
  float GetSomeAvg10() const;

  // Returns the current avg10 "full" value (0-100).
  float GetFullAvg10() const;

  // Force an immediate refresh (for testing or on-demand checks).
  void Refresh();

 private:
  IOPressureMonitor();
  ~IOPressureMonitor() = default;

  IOPressureMonitor(const IOPressureMonitor&) = delete;
  IOPressureMonitor& operator=(const IOPressureMonitor&) = delete;

  void MaybeRefresh() const;
  bool ReadPressureFile();

  mutable std::atomic<float> some_avg10_{0.0f};
  mutable std::atomic<float> full_avg10_{0.0f};
  mutable std::atomic<bool> under_pressure_{false};
  mutable std::atomic<int64_t> last_refresh_us_{0};

  // Path to PSI file (overridable for testing).
  base::FilePath pressure_path_;
  bool psi_available_ = false;
};

}  // namespace cc

#endif  // CC_BASE_IO_PRESSURE_MONITOR_H_
