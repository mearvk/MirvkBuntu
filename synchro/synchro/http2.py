"""HTTP/2 client with a metered (rate-paced) sending path.

The "metered method" is a token-bucket rate limiter that paces outbound
requests to a configurable requests-per-second ceiling with a bounded burst.
This is the honest reading of "metered": deliberately throttled, predictable
egress — useful for large or international dataset transfers where you want to
avoid overrunning a peer or a shared link, and where HTTP/2 multiplexing lets
many streams share one connection.

Every request is timestamped; per-host timing is fed into the same LatencyStats
used by the UDP path, so HTTP results are reported with the same measured
min/p50/p95/p99/jitter/loss semantics — no guarantees, only observations.

Requires the optional ``httpx[http2]`` dependency for live requests. The module
imports without it; the pacing logic (RateMeter) is dependency-free and unit
testable on its own.
"""

from __future__ import annotations

import threading
import time
from dataclasses import dataclass
from typing import Dict, Iterable, List, Optional, Tuple

from .stats import LatencyStats, Sample

try:  # optional dependency
    import httpx  # type: ignore

    _HAVE_HTTPX = True
except Exception:  # pragma: no cover - env without httpx
    httpx = None  # type: ignore
    _HAVE_HTTPX = False


class RateMeter:
    """A thread-safe token-bucket for metered egress.

    Parameters
    ----------
    rate_per_s:
        Sustained requests per second (token refill rate).
    burst:
        Maximum tokens that can accumulate (burst allowance). Defaults to
        ``max(1, rate_per_s)``.
    """

    def __init__(self, rate_per_s: float, *, burst: Optional[float] = None) -> None:
        if rate_per_s <= 0:
            raise ValueError("rate_per_s must be positive")
        self.rate = float(rate_per_s)
        self.capacity = float(burst) if burst is not None else max(1.0, self.rate)
        self._tokens = self.capacity
        self._last = time.monotonic()
        self._lock = threading.Lock()

    def _refill(self) -> None:
        now = time.monotonic()
        elapsed = now - self._last
        self._last = now
        self._tokens = min(self.capacity, self._tokens + elapsed * self.rate)

    def acquire(self, tokens: float = 1.0) -> float:
        """Block until ``tokens`` are available. Returns seconds spent waiting."""
        waited = 0.0
        while True:
            with self._lock:
                self._refill()
                if self._tokens >= tokens:
                    self._tokens -= tokens
                    return waited
                deficit = tokens - self._tokens
                sleep_for = deficit / self.rate
            time.sleep(sleep_for)
            waited += sleep_for


@dataclass
class Http2Response:
    url: str
    status: Optional[int]
    elapsed_ms: float
    error: Optional[str] = None
    wait_ms: float = 0.0

    @property
    def ok(self) -> bool:
        return self.error is None and self.status is not None and 200 <= self.status < 400


class MeteredHttp2Client:
    """HTTP/2 client that paces requests through a RateMeter.

    Parameters
    ----------
    rate_per_s:
        Metered egress ceiling (requests/second).
    burst:
        Token-bucket burst allowance.
    timeout_s:
        Per-request timeout.
    window:
        Retained latency sample window per host.
    """

    def __init__(
        self,
        *,
        rate_per_s: float = 10.0,
        burst: Optional[float] = None,
        timeout_s: float = 10.0,
        window: int = 1024,
    ) -> None:
        self.meter = RateMeter(rate_per_s, burst=burst)
        self.timeout_s = timeout_s
        self.window = window
        self.stats: Dict[str, LatencyStats] = {}
        self._client = None
        self._seq = 0

    # backend protocol -----------------------------------------------------
    def open(self) -> "MeteredHttp2Client":
        if not _HAVE_HTTPX:
            raise RuntimeError(
                "MeteredHttp2Client requires the optional dependency "
                "'httpx[http2]'. Install it to perform live HTTP/2 requests; "
                "RateMeter can be used and tested without it."
            )
        if self._client is None:
            self._client = httpx.Client(  # type: ignore[attr-defined]
                http2=True, timeout=self.timeout_s
            )
        return self

    def close(self) -> None:
        if self._client is not None:
            self._client.close()
            self._client = None

    def __enter__(self):
        return self.open()

    def __exit__(self, *exc):
        self.close()

    def _host_key(self, url: str) -> str:
        try:
            from urllib.parse import urlparse

            p = urlparse(url)
            return p.netloc or url
        except Exception:
            return url

    def _stats_for(self, key: str) -> LatencyStats:
        if key not in self.stats:
            self.stats[key] = LatencyStats(key, window=self.window)
        return self.stats[key]

    def send(self, dest: str, payload: bytes = b"", *, method: str = "GET", **opts):
        """Backend-protocol entry point: one metered request to ``dest`` (a URL)."""
        return self.get(dest, method=method, **opts)

    def get(self, url: str, *, method: str = "GET", **kwargs) -> Http2Response:
        self.open()
        key = self._host_key(url)
        st = self._stats_for(key)
        self._seq += 1
        seq = self._seq

        wait_s = self.meter.acquire(1.0)  # metered pacing happens here
        sent = time.monotonic_ns()
        try:
            resp = self._client.request(method, url, **kwargs)  # type: ignore[union-attr]
            elapsed_ms = (time.monotonic_ns() - sent) / 1e6
            st.record(Sample(key, elapsed_ms, sent / 1e9, seq))
            return Http2Response(
                url=url,
                status=resp.status_code,
                elapsed_ms=elapsed_ms,
                wait_ms=wait_s * 1e3,
            )
        except Exception as exc:  # network error => record as loss
            elapsed_ms = (time.monotonic_ns() - sent) / 1e6
            st.record(Sample(key, None, sent / 1e9, seq))
            return Http2Response(
                url=url,
                status=None,
                elapsed_ms=elapsed_ms,
                error=str(exc),
                wait_ms=wait_s * 1e3,
            )

    def run(self, urls: Iterable[str], *, method: str = "GET") -> List[Http2Response]:
        """Send a metered sequence of requests and return per-request results."""
        results: List[Http2Response] = []
        try:
            for u in urls:
                results.append(self.get(u, method=method))
        finally:
            self.close()
        return results
