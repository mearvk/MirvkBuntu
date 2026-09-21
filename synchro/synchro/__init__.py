"""
Synchro — an honest low-latency packet dispatch and measurement layer.

Synchro does NOT guarantee delivery times. The internet is best-effort and the
speed of light imposes a hard physical floor on latency (light travels ~200 km
per millisecond in fiber, so a host 2,000 km away is >=10 ms away one-way before
any routing or queuing). Instead of asserting a fixed guarantee, Synchro
*measures* real send/ack timing per destination and reports the true, testable
distribution: min / p50 / p95 / p99, jitter, and loss.

The "maximum accuracy we can honestly claim" is therefore an empirical, per-run,
per-destination number computed from observed data — never a hard-coded promise.

Public surface:
    - UdpDispatcher        : timestamped UDP send/ack packet dispatch
    - LatencyStats         : streaming per-destination latency statistics
    - SlaReporter          : "% of hosts meeting a <T ms threshold" report
    - synchro (decorator)  : mark a function as backed by a Synchro backend
    - load_backend         : dynamically import/instantiate a backend by name
    - MeteredHttp2Client   : HTTP/2 client with paced ("metered") sending
"""

from .stats import LatencyStats, Sample
from .dispatcher import UdpDispatcher, DispatchResult
from .sla import SlaReporter, SlaReport
from .backend import synchro, load_backend, Backend, SynchroError

__all__ = [
    "LatencyStats",
    "Sample",
    "UdpDispatcher",
    "DispatchResult",
    "SlaReporter",
    "SlaReport",
    "synchro",
    "load_backend",
    "Backend",
    "SynchroError",
]

__version__ = "0.1.0"
