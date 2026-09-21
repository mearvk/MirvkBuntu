# Synchro Documentation

Reference documentation for the Synchro packet-dispatch and measurement layer.
For a quick overview and install/test instructions, start with the package
[README](../README.md).

## Contents

| Document | What it covers |
|----------|----------------|
| [ARCHITECTURE.md](ARCHITECTURE.md) | Layered design, the `Sample` contract, data flow, threading, and bounded-memory accounting. |
| [ACCURACY.md](ACCURACY.md) | The measurement/precision model: the speed-of-light floor, best-effort delivery, what each metric means, and how to state the strongest *honest* claim. |
| [BACKENDS.md](BACKENDS.md) | The backend protocol, dynamic loading via `load_backend`, the `@synchro` annotation, and writing a custom backend. |
| [HTTP2_METERED.md](HTTP2_METERED.md) | The token-bucket metered HTTP/2 client and `RateMeter`, tuned for large / cross-region dataset transfers. |
| [CLI.md](CLI.md) | Full `python -m synchro.cli` reference, JSON output, and the loopback demo. |

## The one-paragraph summary

Synchro sends timestamped packets (UDP) or paced requests (HTTP/2), matches
replies, and records each result as a `Sample` into per-destination
`LatencyStats`. From those it reports measured `min/p50/p95/p99`, jitter, and
loss, and — via `SlaReporter` — the observed fraction of destinations and
samples meeting a latency threshold. It reports **measured** latency, never a
delivery-time guarantee, because the speed of light and the best-effort Internet
make a fixed sub-10 ms promise to arbitrary hosts physically impossible. See
[ACCURACY.md](ACCURACY.md) for the full reasoning.
