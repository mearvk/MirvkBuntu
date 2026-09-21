"""Streaming per-destination latency statistics.

All latencies are stored and reported in milliseconds. Percentiles are computed
with a "nearest-rank" method over the retained sample window. No latency figure
here is a guarantee — every number is derived from observed samples.
"""

from __future__ import annotations

import math
from collections import deque
from dataclasses import dataclass, field
from typing import Deque, Optional


@dataclass(frozen=True)
class Sample:
    """A single observed round-trip measurement (or a loss)."""

    dest: str
    rtt_ms: Optional[float]  # None => the packet was lost / timed out
    sent_at: float           # monotonic seconds
    seq: int


@dataclass
class LatencyStats:
    """Streaming latency statistics for one destination.

    Keeps the most recent ``window`` successful RTT samples for percentile
    estimation and tracks send/loss counts over the full lifetime.
    """

    dest: str
    window: int = 1024
    _rtts: Deque[float] = field(default_factory=deque, init=False, repr=False)
    sent: int = 0
    acked: int = 0
    lost: int = 0
    _last_rtt: Optional[float] = None
    _sum: float = 0.0
    _sumsq: float = 0.0

    def record(self, sample: Sample) -> None:
        self.sent += 1
        if sample.rtt_ms is None:
            self.lost += 1
            return
        self.acked += 1
        rtt = float(sample.rtt_ms)
        self._rtts.append(rtt)
        self._sum += rtt
        self._sumsq += rtt * rtt
        if len(self._rtts) > self.window:
            old = self._rtts.popleft()
            self._sum -= old
            self._sumsq -= old * old
        self._last_rtt = rtt

    # --- derived metrics -------------------------------------------------
    @property
    def loss_rate(self) -> float:
        return (self.lost / self.sent) if self.sent else 0.0

    @property
    def delivery_rate(self) -> float:
        return (self.acked / self.sent) if self.sent else 0.0

    @property
    def n(self) -> int:
        return len(self._rtts)

    @property
    def mean(self) -> Optional[float]:
        return (self._sum / self.n) if self.n else None

    @property
    def stdev(self) -> Optional[float]:
        if self.n < 2:
            return None
        var = (self._sumsq - (self._sum * self._sum) / self.n) / (self.n - 1)
        return math.sqrt(var) if var > 0 else 0.0

    @property
    def jitter(self) -> Optional[float]:
        """Jitter == stdev of retained RTTs (ms)."""
        return self.stdev

    @property
    def min(self) -> Optional[float]:
        return min(self._rtts) if self._rtts else None

    @property
    def max(self) -> Optional[float]:
        return max(self._rtts) if self._rtts else None

    def percentile(self, p: float) -> Optional[float]:
        """Nearest-rank percentile of retained RTTs. ``p`` in [0, 100]."""
        if not self._rtts:
            return None
        if not 0 <= p <= 100:
            raise ValueError("percentile must be in [0, 100]")
        ordered = sorted(self._rtts)
        if p == 0:
            return ordered[0]
        rank = math.ceil((p / 100.0) * len(ordered))
        return ordered[min(rank, len(ordered)) - 1]

    @property
    def p50(self) -> Optional[float]:
        return self.percentile(50)

    @property
    def p95(self) -> Optional[float]:
        return self.percentile(95)

    @property
    def p99(self) -> Optional[float]:
        return self.percentile(99)

    def as_dict(self) -> dict:
        return {
            "dest": self.dest,
            "sent": self.sent,
            "acked": self.acked,
            "lost": self.lost,
            "delivery_rate": round(self.delivery_rate, 6),
            "loss_rate": round(self.loss_rate, 6),
            "samples": self.n,
            "min_ms": _r(self.min),
            "p50_ms": _r(self.p50),
            "p95_ms": _r(self.p95),
            "p99_ms": _r(self.p99),
            "max_ms": _r(self.max),
            "mean_ms": _r(self.mean),
            "jitter_ms": _r(self.jitter),
        }


def _r(x: Optional[float]) -> Optional[float]:
    return round(x, 4) if x is not None else None
