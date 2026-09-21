# Accuracy and Precision in Synchro

Synchro's central design decision is to **measure and report** latency rather
than promise it. This document explains what the numbers mean, why a fixed
"delivery within 0.01 s at 99.2%" guarantee is not offered, and how to state the
strongest claim that is actually true for a given run.

## The physical floor

Latency to a destination has a lower bound set by physics that no software can
beat:

- Light in a vacuum travels ~300,000 km/s. In optical fiber the effective
  signal speed is roughly **~200 km/ms** (≈2/3 of c, due to the fiber's
  refractive index).
- Therefore a destination `D` kilometers away has a one-way propagation floor of
  about `D / 200` ms, and a round-trip floor of about `D / 100` ms — **before**
  any routing, queuing, serialization, or processing.

Some worked examples of the *round-trip* propagation floor (fiber, great-circle
approximation — real paths are longer):

| Path (approx. distance) | RTT floor (≈ dist / 100 km·ms⁻¹) |
|-------------------------|----------------------------------|
| Within a metro (~50 km) | ~0.5 ms |
| ~500 km (regional)      | ~5 ms |
| ~2,000 km (cross-country) | ~20 ms |
| ~10,000 km (intercontinental) | ~100 ms |

A blanket "within 0.01 s (10 ms) to any known IP" is therefore impossible for
any destination more than ~1,000 km away, purely from geometry. This is why
Synchro reports a measured figure instead of a fixed one.

## Best-effort delivery

Even within the physical floor, the public Internet gives **no delivery
guarantee**. Packets can be dropped, reordered, delayed by transient
congestion, or blackholed by a route change. A "99.2% guaranteed" figure to
destinations you do not control is not something any layer can honestly promise.
What you *can* do is **observe** the delivered fraction and report it — which is
exactly what `LatencyStats.delivery_rate` and `SlaReporter` do.

## What Synchro measures

For each destination, over a retained window of recent samples:

| Metric | Definition |
|--------|------------|
| `min` / `max` | Smallest / largest retained RTT (ms). |
| `p50`, `p95`, `p99` | Nearest-rank percentiles of retained RTTs (ms). |
| `mean` | Arithmetic mean of retained RTTs (ms). |
| `jitter` | Sample standard deviation of retained RTTs (ms). |
| `delivery_rate` | `acked / sent` over the destination's lifetime. |
| `loss_rate` | `lost / sent` over the destination's lifetime. |

### Percentile method

Synchro uses the **nearest-rank** percentile: for percentile `p` over `n` sorted
samples, it takes the value at rank `ceil(p/100 · n)` (1-indexed). This is exact
for the retained window, needs no interpolation, and is deterministic. Because
the window is bounded (default 1024 samples), `p99` is meaningful only once you
have enough samples — with `n < 100`, `p99` effectively reports the current
maximum. Collect enough rounds for the percentile you care about (e.g. ≥1000
samples for a stable `p99`).

### Jitter

`jitter` is the sample standard deviation (`ddof=1`) of the retained RTTs,
computed from running sums so it stays O(1) per update. It is `None` until at
least two samples exist.

## Stating the strongest honest claim

Instead of a fixed guarantee, derive the claim from the run:

1. Choose a threshold `T` (ms) and a percentile `p` that matter for your use.
2. Run enough samples per destination for `p` to be meaningful.
3. Read the `SlaReport`:
   - **Destination compliance** — fraction of destinations whose `p`-th
     percentile RTT ≤ `T`.
   - **Sample compliance** — fraction of individual successful samples with
     RTT ≤ `T`.

A truthful statement then looks like:

> "Over this run, 99.4% of samples to the measured hosts were delivered within
> 5.0 ms (p99 ≤ 5.0 ms for 47/48 destinations); overall loss was 0.1%."

That is a real, reproducible, per-run claim — the honest maximum. It is bounded
by the physical floor above and by the actual network conditions at the time of
measurement, and it will differ (correctly) between loopback, LAN, regional, and
intercontinental targets.

## Sources of measurement error

Numbers are honest but not infinitely precise. Known contributors:

- **Host scheduling / GC pauses** can add sub-millisecond to millisecond noise
  to a userspace RTT measurement, especially under load.
- **Echo-server cost** — a measured RTT includes the responder's turnaround
  time. The built-in loopback echo server is minimal, but a real peer's
  processing time is part of what you observe (and usually what you want).
- **Clock resolution** — `monotonic_ns()` is nanosecond-resolution, but the OS
  timer granularity and syscall overhead dominate at very low RTTs.
- **Window size** — percentiles describe only the retained window, not all-time
  history. Size the window for your reporting needs.

None of these are hidden: they widen the measured distribution, which shows up
honestly in `jitter`, `p95`, and `p99`.
