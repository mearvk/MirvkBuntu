# Synchro

An honest, low-latency **packet dispatch and measurement** layer for MirvkBuntu.

Synchro sends timestamped packets to destinations, matches replies, and reports
the **measured** latency distribution per destination — plus a metered HTTP/2
sending path and a dynamically loaded backend behind a `@synchro` annotation.

## What Synchro does *not* claim

Synchro deliberately makes **no delivery-time guarantee**, because none is
physically possible for arbitrary Internet destinations:

- **The speed of light is a hard floor.** Light in fiber travels ~200 km per
  millisecond. A host 2,000 km away is therefore **≥10 ms away one-way** from
  pure propagation, before any routing, queuing, or serialization. "Delivery to
  any known IP within 0.01 s" is refuted by geography alone for most of the
  planet.
- **The Internet is best-effort.** No software layer can *guarantee* a delivery
  percentage to destinations it does not control. Congestion, loss, re-routing,
  and unreachable hosts are real and unbounded.

So the "maximum accuracy we can honestly claim" is **not a fixed number** — it
is an **empirical, per-run, per-destination** figure that Synchro *measures* and
reports. That is the honest form of precision: what actually happened, to four
decimal places of a millisecond, with the loss rate stated alongside it.

## Measured example (loopback)

Running the built-in echo server and probing it over loopback (`lo`), where you
see the genuine floor of the local network stack:

```text
127.0.0.1:9873: p50=0.0558 p95=0.2865 p99=0.3331 jitter=0.0808 loss=0.00% (n=200)

Synchro SLA report (measured, not guaranteed)
  threshold           : 1.000 ms
  evaluated at        : p99
  destinations        : 1/1 meeting threshold (100.00%)
  samples within thr. : 200/200 (100.00%)
```

Those numbers are produced by the code in this directory, not hand-written.
On a real WAN path the same command will report the real, larger latencies and
any real loss — which is exactly the point.

## Components

| Module | Purpose |
|--------|---------|
| `synchro/dispatcher.py` | `UdpDispatcher` — timestamped UDP send/ack RTT measurement (uses `time.monotonic_ns()`), plus a loopback echo server for self-contained runs. |
| `synchro/stats.py` | `LatencyStats` — streaming per-destination min / p50 / p95 / p99 / max, mean, jitter (stdev), and delivery/loss rates over a retained sample window. |
| `synchro/sla.py` | `SlaReporter` — the testable form of a latency target: the **observed** fraction of destinations (and of samples) that met a `<T ms` threshold at a chosen percentile. |
| `synchro/backend.py` | `load_backend()` (on-demand `importlib` loading) and the `@synchro(...)` annotation that lazily binds a function to a backend. |
| `synchro/http2.py` | `MeteredHttp2Client` + `RateMeter` — HTTP/2 requests paced by a token-bucket ("metered method"), timestamped into the same `LatencyStats`. |
| `synchro/cli.py` | `python -m synchro.cli udp ...` command-line measurement + SLA report. |

## The `@synchro` annotation (dynamic backend loading)

The annotation binds a function to a backend that is **imported and constructed
on first call**, not at import time — "dynamically loads the Synchro backend for
the network exchange":

```python
from synchro import synchro

@synchro("udp", [("127.0.0.1", 9999)])   # backend spec + its constructor args
def probe(*, backend):                    # backend injected lazily as a kwarg
    return backend.run(rounds=100)

report = probe()                          # UdpDispatcher imported/built here
```

Backends are referenced by short name (`udp`, `http2`) or by an
entry-point-style `"module.path:Attribute"` spec, so any importable class that
exposes `send(...)` can be a Synchro backend without prior registration.

## The metered HTTP/2 path

`MeteredHttp2Client` throttles egress through a thread-safe token bucket
(`RateMeter`) — a predictable requests/second ceiling with a bounded burst.
This is the honest reading of "metered": deliberately paced egress, well suited
to large or international dataset transfers over HTTP/2's multiplexed
connection. Live requests need the optional `httpx[http2]` extra; the pacing
logic itself is dependency-free and unit-tested.

```python
from synchro.http2 import MeteredHttp2Client

with MeteredHttp2Client(rate_per_s=20, burst=5) as c:
    for r in [c.get("https://example.com/data") for _ in range(100)]:
        print(r.status, round(r.elapsed_ms, 2), "ms  waited", round(r.wait_ms, 2))
```

## CLI

```bash
# Terminal 1 — loopback echo server
SYNCHRO_ECHO_MAIN=1 SYNCHRO_ECHO_PORT=9999 python -m synchro.dispatcher

# Terminal 2 — measure and evaluate against a 1 ms p99 target
python -m synchro.cli udp --dest 127.0.0.1:9999 --rounds 200 \
    --sla-threshold-ms 1.0 --sla-percentile 99
```

Add `--json` for machine-readable output.

## Install & test

```bash
pip install -e synchro                 # or: pip install -e 'synchro[http2,test]'
PYTHONPATH=synchro pytest synchro/tests -q
```

The test suite is self-contained (no external network): it spins up the
loopback echo server and measures genuine RTTs end to end.

## Further documentation

Deeper references live in [`docs/`](docs/):

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — layered design and the `Sample` contract.
- [docs/ACCURACY.md](docs/ACCURACY.md) — the measurement/precision model and the physical floor.
- [docs/BACKENDS.md](docs/BACKENDS.md) — the backend protocol, dynamic loading, and `@synchro`.
- [docs/HTTP2_METERED.md](docs/HTTP2_METERED.md) — the metered HTTP/2 transfer path.
- [docs/CLI.md](docs/CLI.md) — full CLI reference and JSON output.

## Notes on HTTP/2 and "International Datasets"

For cross-region ("international") dataset movement, the metered HTTP/2 client
is the recommended path: HTTP/2 multiplexing amortizes connection setup across
many streams, and the token-bucket rate limit keeps egress predictable on shared
or long-haul links. Synchro reports the real observed latency for those
transfers with the same measured percentiles as the UDP path — it does not, and
will not, promise a fixed delivery time across such distances.
