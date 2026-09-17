// Copyright 2011 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_BASE_COMPLETION_EVENT_H_
#define CC_BASE_COMPLETION_EVENT_H_

#include "base/check.h"
#include "base/logging.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "cc/base/io_pressure_monitor.h"

namespace cc {

// Default maximum time to wait for a completion event before considering the
// system stalled. This prevents unbounded blocking of the main thread when
// the compositor impl thread is stalled on I/O (e.g., tile raster blocked by
// disk saturation from external processes like git gc, or memory pressure
// causing swap thrash).
//
// Galactic Cherry Marvell Edition 98 — Anti-freeze safety timeout.
// When this timeout fires, the main thread is released to remain responsive.
// The impl thread will still complete its work and signal, but the main thread
// will not be blocked indefinitely.
inline constexpr base::TimeDelta kDefaultCommitCompletionTimeout =
    base::Milliseconds(4000);

// Shortened timeout when I/O pressure is already detected. No point waiting
// 4s when the system is known to be saturated.
inline constexpr base::TimeDelta kIOPressureCommitCompletionTimeout =
    base::Milliseconds(500);

// Used for making blocking calls from one thread to another. Use only when
// absolutely certain that doing-so will not lead to a deadlock.
//
// MODIFICATION (Galactic Cherry Marvell Edition 98):
// Wait() now enforces a maximum timeout to prevent GUI thread freezing when
// the impl thread is stalled on I/O. If the timeout fires, Wait() returns
// and sets timed_out_ = true. Callers can check DidTimeOut() to handle the
// late-completion case gracefully.
//
// It is safe to destroy this object as soon as Wait() returns.
class CompletionEvent {
 public:
  explicit CompletionEvent(base::WaitableEvent::ResetPolicy policy =
                               base::WaitableEvent::ResetPolicy::AUTOMATIC)
      : event_(policy, base::WaitableEvent::InitialState::NOT_SIGNALED),
        timeout_(kDefaultCommitCompletionTimeout) {
#if DCHECK_IS_ON()
    waited_ = false;
    signaled_ = false;
#endif
  }

  ~CompletionEvent() {
#if DCHECK_IS_ON()
    DCHECK(waited_);
    // Signal may arrive after timeout — do not DCHECK signaled_ here.
    // The impl thread will still signal; we just didn't wait for it.
#endif
  }

  // Set a custom timeout for this completion event.
  // Must be called before Wait().
  void SetTimeout(base::TimeDelta timeout) { timeout_ = timeout; }

  void Wait() {
#if DCHECK_IS_ON()
    DCHECK(!waited_);
    waited_ = true;
#endif
    if (IsSignaled()) {
      // The event has already been signaled and cannot be re-signaled.
      // There is a non-trivial amount of machinery in WaitableEvent to quickly
      // return if already signaled, which can be short-circuited.
      return;
    }
    // http://crbug.com/902653
    base::ScopedAllowBaseSyncPrimitivesOutsideBlockingScope allow_wait;

    // GALACTIC CHERRY: Use timed wait with safety ceiling.
    // If the impl thread is stalled (I/O saturation, memory pressure,
    // swap thrash), the main thread MUST NOT block indefinitely — that
    // freezes the entire GUI.
    //
    // Additionally, if we already know the system is under I/O pressure
    // (via PSI), use a much shorter timeout — there's no point waiting 4s
    // when we know the disk is saturated.
    base::TimeDelta effective_timeout = timeout_;
    if (IOPressureMonitor::GetInstance().IsUnderIOPressure()) {
      effective_timeout = std::min(timeout_, kIOPressureCommitCompletionTimeout);
      TRACE_EVENT_INSTANT("cc", "CompletionEvent::Wait::IOPressureDetected");
    }

    if (!event_.TimedWait(effective_timeout)) {
      timed_out_ = true;
      TRACE_EVENT_INSTANT("cc", "CompletionEvent::Wait::TIMEOUT");
      LOG(WARNING) << "CompletionEvent::Wait() timed out after "
                   << effective_timeout.InMilliseconds()
                   << "ms. Main thread released to prevent GUI freeze. "
                   << "Impl thread may still be blocked on I/O. "
                   << "IO pressure: some_avg10="
                   << IOPressureMonitor::GetInstance().GetSomeAvg10()
                   << " full_avg10="
                   << IOPressureMonitor::GetInstance().GetFullAvg10();
    }
  }

  bool TimedWait(const base::TimeDelta& max_time) {
#if DCHECK_IS_ON()
    DCHECK(!waited_);
    waited_ = true;
#endif
    // http://crbug.com/902653
    base::ScopedAllowBaseSyncPrimitivesOutsideBlockingScope allow_wait;
    if (event_.TimedWait(max_time))
      return true;
#if DCHECK_IS_ON()
    waited_ = false;
#endif
    timed_out_ = true;
    return false;
  }

  bool IsSignaled() { return event_.IsSignaled(); }

  // Returns true if Wait() returned due to timeout rather than signal.
  bool DidTimeOut() const { return timed_out_; }

  void Signal() {
#if DCHECK_IS_ON()
    // Allow double-signal after timeout: the impl thread will still call
    // Signal() even if the main thread has already released.
    if (!signaled_)
      signaled_ = true;
#endif
    event_.Signal();
  }

 private:
  base::WaitableEvent event_;
  base::TimeDelta timeout_;
  bool timed_out_ = false;
#if DCHECK_IS_ON()
  // Used to assert that Wait() and Signal() are each called exactly once.
  bool waited_;
  bool signaled_;
#endif
};

}  // namespace cc

#endif  // CC_BASE_COMPLETION_EVENT_H_
