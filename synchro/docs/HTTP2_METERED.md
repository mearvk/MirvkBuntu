# Metered HTTP/2 Transfers

The `MeteredHttp2Client` in `http2.py` is Synchro's transport for HTTP/2
requests whose egress is **paced** ("metered") by a token-bucket rate limiter.
It is the recommended path for large or cross-region ("international") dataset
movement where predictable, throttled egress matters.

## Why "metered"

"Metered" here means deliberately rate-limited, predictable egress — not
best-effort blasting. A token bucket gives you two knobs:

- **`rate_per_s`** — the sustained requests-per-second ceiling (token refill
  rate).
- **`burst`** — the maximum tokens that can accumulate, i.e. how large a
  short-term burst is allowed before pacing kicks in (defaults to
  `max(1, rate_per_s)`).

This keeps you from overrunning a peer, a shared link, or an API quota, and it
makes transfer timing predictable — which in turn makes the measured latency
distribution interpretable.

## Why HTTP/2 for datasets

HTTP/2 multiplexes many logical streams over a single connection, so
per-request connection setup is amortized and head-of-line blocking is reduced
relative to HTTP/1.1. For moving many dataset objects to a distant region, that
amortization plus a steady metered rate is usually the pragmatic choice. Synchro
reports the **observed** latency for those transfers with the same
`min/p50/p95/p99/jitter/loss` semantics as the UDP path — it does not promise a
fixed delivery time across long-haul distances (see [ACCURACY.md](ACCURACY.md)).

## RateMeter (dependency-free)

`RateMeter` is a thread-safe token bucket usable on its own, with no HTTP
dependency:

```python
from synchro.http2 import RateMeter

meter = RateMeter(rate_per_s=50, burst=10)
waited_s = meter.acquire(1.0)   # blocks if necessary; returns time spent waiting
```

Semantics:

- Tokens refill continuously at `rate_per_s`.
- `acquire(n)` blocks until `n` tokens are available, then consumes them, and
  returns the seconds it waited (0.0 if tokens were already available).
- The bucket never exceeds `burst` tokens, bounding how much a quiet period can
  be "cashed in" as a later burst.

## Client usage

Live requests require the optional `httpx[http2]` extra. The pacing logic works
without it (and is unit-tested without it).

```python
from synchro.http2 import MeteredHttp2Client

# 20 req/s sustained, allow bursts up to 5
with MeteredHttp2Client(rate_per_s=20, burst=5, timeout_s=10) as client:
    urls = [f"https://data.example.com/chunk/{i}" for i in range(1000)]
    results = [client.get(u) for u in urls]

    ok = sum(1 for r in results if r.ok)
    print(f"{ok}/{len(results)} ok")

    # Same measured stats as the UDP path:
    for host, st in client.stats.items():
        d = st.as_dict()
        print(host, "p50", d["p50_ms"], "p99", d["p99_ms"], "loss", d["loss_rate"])
```

Each call returns an `Http2Response`:

| Field | Meaning |
|-------|---------|
| `url` | The requested URL. |
| `status` | HTTP status code, or `None` on a transport error. |
| `elapsed_ms` | Measured request round-trip in milliseconds. |
| `wait_ms` | Time spent waiting on the rate limiter before sending. |
| `error` | Error string if the request failed (recorded as a loss). |
| `ok` (property) | `True` when there was no error and status is 2xx/3xx. |

## Feeding an SLA report

Because the client accumulates `LatencyStats` per host, you can evaluate the
same SLA report used for UDP:

```python
from synchro import SlaReporter

report = SlaReporter(threshold_ms=250, percentile=95).evaluate(client.stats)
print(report.summary())
```

## Tuning notes

- **Match `rate_per_s` to the slowest shared resource** (peer quota, link
  capacity, or your own CPU for TLS), not to the fastest.
- **Use a modest `burst`** to smooth bursts without defeating the meter; a burst
  equal to a second or two of rate is a common starting point.
- **Interpret `wait_ms`**: high `wait_ms` with low `elapsed_ms` means the meter
  (your policy), not the network, is the bottleneck — usually intended.
- **Errors are losses**: a failed request is recorded as a loss sample, so it
  correctly lowers `delivery_rate` rather than silently vanishing.
