"""Self-contained tests for Synchro. No network egress required.

The UDP test runs a real loopback echo server in a thread and measures genuine
RTTs, so it validates the whole send/ack/timing/stats path end to end.
"""

import socket
import threading
import time

import pytest

from synchro import (
    LatencyStats,
    Sample,
    SlaReporter,
    UdpDispatcher,
    load_backend,
    synchro,
)
from synchro.dispatcher import run_echo_server
from synchro.http2 import RateMeter


def test_stats_percentiles_and_loss():
    s = LatencyStats("d")
    for i in range(1, 101):  # 1..100 ms
        s.record(Sample("d", float(i), 0.0, i))
    s.record(Sample("d", None, 0.0, 101))  # one loss
    assert s.min == 1.0
    assert s.max == 100.0
    assert s.p50 == 50.0
    assert s.p95 == 95.0
    assert s.p99 == 99.0
    assert s.sent == 101 and s.acked == 100 and s.lost == 1
    assert abs(s.loss_rate - (1 / 101)) < 1e-9


def _free_port() -> int:
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(("127.0.0.1", 0))
    port = s.getsockname()[1]
    s.close()
    return port


def test_udp_roundtrip_loopback():
    port = _free_port()
    rounds = 50
    t = threading.Thread(
        target=run_echo_server, args=(("127.0.0.1", port),),
        kwargs={"stop_after": rounds}, daemon=True,
    )
    t.start()
    time.sleep(0.1)

    disp = UdpDispatcher([("127.0.0.1", port)], timeout_s=0.5)
    result = disp.run(rounds=rounds, interval_s=0.001)
    key = f"127.0.0.1:{port}"
    s = result.stats[key]
    # On loopback essentially everything should be delivered and measurable.
    assert s.acked >= rounds - 2
    assert s.p99 is not None and s.p99 >= 0.0


def test_sla_reporter():
    s = LatencyStats("d")
    for i in range(100):
        s.record(Sample("d", 0.5, 0.0, i))  # all 0.5 ms
    rep = SlaReporter(1.0, percentile=99).evaluate({"d": s})
    assert rep.dests_meeting == 1
    assert rep.dest_compliance == 1.0
    assert rep.sample_compliance == 1.0

    rep2 = SlaReporter(0.1, percentile=99).evaluate({"d": s})
    assert rep2.dests_meeting == 0


def test_rate_meter_paces():
    m = RateMeter(rate_per_s=50.0, burst=1.0)
    m.acquire(1.0)  # drains initial token
    start = time.monotonic()
    m.acquire(1.0)  # must wait ~1/50 s
    elapsed = time.monotonic() - start
    assert elapsed >= 0.015  # allow scheduler slack around the 0.02s target


def test_backend_lazy_loading_and_annotation():
    disp = load_backend("udp", [("127.0.0.1", 9)], cache=False)
    assert isinstance(disp, UdpDispatcher)

    @synchro("udp", [("127.0.0.1", 9)], cache=False)
    def probe(*, backend):
        return type(backend).__name__

    assert probe() == "UdpDispatcher"
    assert probe.synchro_backend == "udp"


def test_unknown_backend_raises():
    from synchro import SynchroError

    with pytest.raises(SynchroError):
        load_backend("does-not-exist")
