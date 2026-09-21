# Synchro Architecture

Synchro is a small, layered measurement library. Each layer has one job and
depends only on the layer(s) below it, so the pieces can be used and tested in
isolation.

```text
                 ┌───────────────────────────────────────────┐
   entry points  │  cli.py            @synchro / load_backend │
                 └───────┬───────────────────────┬────────────┘
                         │                        │
              ┌──────────▼──────────┐   ┌─────────▼───────────┐
   transports │  UdpDispatcher      │   │  MeteredHttp2Client │
              │  (dispatcher.py)    │   │  (http2.py)         │
              │                     │   │  + RateMeter        │
              └──────────┬──────────┘   └─────────┬───────────┘
                         │                        │
                         └───────────┬────────────┘
                                     │ Sample(dest, rtt_ms|None, sent_at, seq)
                          ┌──────────▼──────────┐
              accounting  │  LatencyStats       │  (stats.py)
                          │  min/p50/p95/p99…   │
                          └──────────┬──────────┘
                                     │ Dict[str, LatencyStats]
                          ┌──────────▼──────────┐
              reporting   │  SlaReporter        │  (sla.py)
                          │  measured % ≤ T ms  │
                          └─────────────────────┘
```

## Data flow

1. A **transport** (`UdpDispatcher` or `MeteredHttp2Client`) sends a packet or
   request and records the send time with `time.monotonic_ns()`.
2. When a reply is matched (UDP: by sequence number in the echoed header;
   HTTP/2: by the returned response), the transport computes the round-trip
   time and produces a `Sample`. A miss/timeout/error produces a `Sample` with
   `rtt_ms=None`, i.e. a recorded **loss**.
3. Each `Sample` is fed into the destination's `LatencyStats`, which maintains a
   bounded, streaming view: a fixed-size window of recent RTTs for percentiles,
   plus lifetime `sent`/`acked`/`lost` counters and running sum/sum-of-squares
   for mean and standard deviation (jitter).
4. `SlaReporter` reads the per-destination `LatencyStats` and evaluates them
   against a threshold at a chosen percentile, producing an `SlaReport`.

## Why `monotonic_ns`

Wall-clock time (`time.time()`) can jump backward or forward (NTP steps, manual
clock changes, DST). `time.monotonic_ns()` never goes backward and has
nanosecond resolution, so an interval measured as `recv_ns - sent_ns` is always
a valid, non-negative duration. All RTTs are derived this way and reported in
milliseconds.

## The `Sample` contract

`Sample` (in `stats.py`) is the single unit of measurement passed between
layers:

| Field | Meaning |
|-------|---------|
| `dest` | Destination key (`"ip:port"` for UDP, host `netloc` for HTTP/2). |
| `rtt_ms` | Measured round-trip in milliseconds, or `None` for a loss. |
| `sent_at` | Monotonic send time in seconds (bookkeeping/ordering). |
| `seq` | Monotonic per-transport sequence number. |

Any transport that produces `Sample`s can reuse the whole accounting and
reporting stack unchanged — this is what keeps the UDP and HTTP/2 paths
consistent.

## Bounded memory

`LatencyStats` keeps at most `window` RTTs (default 1024) per destination. When
the window is full the oldest RTT is evicted and the running sums are adjusted,
so memory per destination is constant regardless of how long a run lasts. The
lifetime counters (`sent`/`acked`/`lost`) are exact and unbounded-count safe.

## Threading

- `UdpDispatcher` is single-threaded per `run()` call and opens/closes one
  socket per run.
- `RateMeter` (used by the HTTP/2 path) is thread-safe via an internal lock, so
  a single meter can pace requests issued from multiple threads.
- `load_backend()` caches instances behind a lock so concurrent first-calls
  resolve to one shared backend instance (unless `cache=False`).

See [ACCURACY.md](ACCURACY.md) for the measurement/precision model,
[BACKENDS.md](BACKENDS.md) for writing a custom backend, and
[HTTP2_METERED.md](HTTP2_METERED.md) for the metered transfer path.
