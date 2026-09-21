"""Timestamped UDP packet dispatch.

The dispatcher sends probe packets to a set of destinations and matches echoed
replies to measure round-trip time (RTT). It uses ``time.monotonic_ns()`` for
timing so measurements are immune to wall-clock adjustments.

Honesty note: RTT is *measured*, never promised. If a destination does not echo
(most hosts won't reply to an arbitrary UDP probe), the packet is recorded as
lost — which is itself an honest, useful data point.
"""

from __future__ import annotations

import os
import socket
import struct
import time
from dataclasses import dataclass
from typing import Dict, Iterable, List, Optional, Tuple

from .stats import LatencyStats, Sample

_MAGIC = b"SYNC"  # 4-byte tag so we only match our own echoes
_HDR = struct.Struct("!4sIQ")  # magic, seq (u32), sent_ns (u64)


@dataclass
class DispatchResult:
    """Outcome of a dispatch run keyed by destination."""

    stats: Dict[str, LatencyStats]

    def as_dict(self) -> dict:
        return {dest: s.as_dict() for dest, s in self.stats.items()}


def _pack(seq: int, sent_ns: int, payload_len: int) -> bytes:
    hdr = _HDR.pack(_MAGIC, seq & 0xFFFFFFFF, sent_ns & 0xFFFFFFFFFFFFFFFF)
    pad = payload_len - len(hdr)
    if pad > 0:
        hdr = hdr + (b"\x00" * pad)
    return hdr


def _unpack(data: bytes) -> Optional[Tuple[int, int]]:
    if len(data) < _HDR.size:
        return None
    magic, seq, sent_ns = _HDR.unpack(data[: _HDR.size])
    if magic != _MAGIC:
        return None
    return seq, sent_ns


class UdpDispatcher:
    """Send timestamped UDP probes to destinations and measure RTT.

    Parameters
    ----------
    destinations:
        Iterable of ``(ip, port)`` tuples.
    payload_bytes:
        Total datagram size (>= header size). Larger payloads exercise MTU/
        fragmentation behavior.
    timeout_s:
        Per-receive-poll timeout; also the effective loss cutoff for a round.
    window:
        Retained sample window per destination (see LatencyStats).
    """

    def __init__(
        self,
        destinations: Iterable[Tuple[str, int]],
        *,
        payload_bytes: int = 64,
        timeout_s: float = 0.5,
        window: int = 1024,
    ) -> None:
        self.destinations: List[Tuple[str, int]] = list(destinations)
        if payload_bytes < _HDR.size:
            payload_bytes = _HDR.size
        self.payload_bytes = payload_bytes
        self.timeout_s = timeout_s
        self.stats: Dict[str, LatencyStats] = {
            self._key(d): LatencyStats(self._key(d), window=window)
            for d in self.destinations
        }
        self._seq = 0

    @staticmethod
    def _key(dest: Tuple[str, int]) -> str:
        return f"{dest[0]}:{dest[1]}"

    def run(self, rounds: int = 1, *, interval_s: float = 0.0) -> DispatchResult:
        """Send ``rounds`` probes to every destination and collect RTTs.

        A UDP socket is opened per run. In each round every destination gets one
        probe; replies are drained after the batch and matched by sequence.
        Unmatched probes are recorded as lost once the round's timeout elapses.
        """
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setblocking(False)
        try:
            for _ in range(rounds):
                inflight: Dict[int, Tuple[str, float, int]] = {}
                for dest in self.destinations:
                    seq = self._next_seq()
                    sent_ns = time.monotonic_ns()
                    pkt = _pack(seq, sent_ns, self.payload_bytes)
                    try:
                        sock.sendto(pkt, dest)
                    except OSError:
                        # Immediate send failure counts as a loss for this dest.
                        self.stats[self._key(dest)].record(
                            Sample(self._key(dest), None, sent_ns / 1e9, seq)
                        )
                        continue
                    inflight[seq] = (self._key(dest), sent_ns, seq)

                self._drain(sock, inflight)

                # Anything still inflight after the timeout window is lost.
                for key, sent_ns, seq in inflight.values():
                    self.stats[key].record(Sample(key, None, sent_ns / 1e9, seq))

                if interval_s > 0:
                    time.sleep(interval_s)
        finally:
            sock.close()
        return DispatchResult(self.stats)

    def _drain(self, sock: socket.socket, inflight: Dict) -> None:
        deadline = time.monotonic() + self.timeout_s
        while inflight and time.monotonic() < deadline:
            remaining = deadline - time.monotonic()
            try:
                import select

                r, _, _ = select.select([sock], [], [], max(0.0, remaining))
                if not r:
                    break
                data, _addr = sock.recvfrom(65535)
            except (BlockingIOError, InterruptedError):
                continue
            except OSError:
                break
            parsed = _unpack(data)
            if parsed is None:
                continue
            seq, echoed_sent_ns = parsed
            entry = inflight.pop(seq, None)
            if entry is None:
                continue
            key, sent_ns, _ = entry
            recv_ns = time.monotonic_ns()
            rtt_ms = (recv_ns - sent_ns) / 1e6
            self.stats[key].record(Sample(key, rtt_ms, sent_ns / 1e9, seq))

    def _next_seq(self) -> int:
        self._seq = (self._seq + 1) & 0xFFFFFFFF
        return self._seq


def run_echo_server(bind: Tuple[str, int], *, stop_after: Optional[int] = None) -> None:
    """Minimal UDP echo server used for local, self-contained measurement.

    Echoes back any datagram carrying the Synchro magic header. Handy for tests
    and for demonstrating real RTT numbers on loopback (where you'll see the
    genuine sub-millisecond floor of the local stack).
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(bind)
    served = 0
    try:
        while True:
            data, addr = sock.recvfrom(65535)
            if _unpack(data) is not None:
                sock.sendto(data, addr)
                served += 1
                if stop_after is not None and served >= stop_after:
                    return
    finally:
        sock.close()


if os.environ.get("SYNCHRO_ECHO_MAIN"):  # pragma: no cover - manual helper
    host = os.environ.get("SYNCHRO_ECHO_HOST", "127.0.0.1")
    port = int(os.environ.get("SYNCHRO_ECHO_PORT", "9999"))
    run_echo_server((host, port))
