"""SLA-style reporting: the honest, testable form of a latency target.

Instead of claiming "delivery within T at X% guaranteed", the SlaReporter takes
measured stats and reports the *observed* fraction of destinations (and of
individual samples) that met a latency threshold over the run. Every figure is
computed from data — it describes what happened, not what is promised.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Dict, List

from .stats import LatencyStats


@dataclass
class SlaReport:
    threshold_ms: float
    percentile: float
    total_dests: int
    dests_meeting: int
    per_dest: Dict[str, dict] = field(default_factory=dict)
    total_samples: int = 0
    samples_within_threshold: int = 0

    @property
    def dest_compliance(self) -> float:
        """Fraction of destinations whose chosen percentile RTT <= threshold."""
        return (self.dests_meeting / self.total_dests) if self.total_dests else 0.0

    @property
    def sample_compliance(self) -> float:
        """Fraction of individual successful samples with RTT <= threshold."""
        return (
            self.samples_within_threshold / self.total_samples
            if self.total_samples
            else 0.0
        )

    def summary(self) -> str:
        lines = [
            "Synchro SLA report (measured, not guaranteed)",
            f"  threshold           : {self.threshold_ms:.3f} ms",
            f"  evaluated at        : p{self.percentile:g}",
            f"  destinations        : {self.dests_meeting}/{self.total_dests} "
            f"meeting threshold ({self.dest_compliance * 100:.2f}%)",
            f"  samples within thr. : {self.samples_within_threshold}/"
            f"{self.total_samples} ({self.sample_compliance * 100:.2f}%)",
        ]
        return "\n".join(lines)

    def as_dict(self) -> dict:
        return {
            "threshold_ms": self.threshold_ms,
            "percentile": self.percentile,
            "total_dests": self.total_dests,
            "dests_meeting": self.dests_meeting,
            "dest_compliance": round(self.dest_compliance, 6),
            "total_samples": self.total_samples,
            "samples_within_threshold": self.samples_within_threshold,
            "sample_compliance": round(self.sample_compliance, 6),
            "per_dest": self.per_dest,
        }


class SlaReporter:
    """Evaluate measured latency stats against a threshold.

    Parameters
    ----------
    threshold_ms:
        Latency target in milliseconds.
    percentile:
        Percentile at which each destination is judged (default p99). A
        destination "meets" the SLA if its p<percentile> RTT <= threshold.
    """

    def __init__(self, threshold_ms: float, *, percentile: float = 99.0) -> None:
        if threshold_ms <= 0:
            raise ValueError("threshold_ms must be positive")
        self.threshold_ms = float(threshold_ms)
        self.percentile = float(percentile)

    def evaluate(self, stats: Dict[str, LatencyStats]) -> SlaReport:
        report = SlaReport(
            threshold_ms=self.threshold_ms,
            percentile=self.percentile,
            total_dests=len(stats),
            dests_meeting=0,
        )
        for dest, s in stats.items():
            pval = s.percentile(self.percentile)
            meets = pval is not None and pval <= self.threshold_ms
            if meets:
                report.dests_meeting += 1
            # Count sample-level compliance from retained window.
            within = sum(1 for r in s._rtts if r <= self.threshold_ms)  # noqa: SLF001
            report.total_samples += s.n
            report.samples_within_threshold += within
            d = s.as_dict()
            d["meets_threshold"] = bool(meets)
            d["percentile_value_ms"] = round(pval, 4) if pval is not None else None
            report.per_dest[dest] = d
        return report
