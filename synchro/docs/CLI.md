# Synchro CLI Reference

The command-line interface runs measurements and prints either a human-readable
summary or JSON. Invoke it as a module:

```bash
python -m synchro.cli <command> [options]
```

The banner is intentional: **"Measured latency, not guaranteed."**

## `udp` — UDP RTT measurement

Sends timestamped UDP probes to one or more destinations and reports the
measured latency distribution, optionally evaluated against an SLA threshold.

```bash
python -m synchro.cli udp --dest HOST:PORT [--dest HOST:PORT ...] [options]
```

### Options

| Option | Type | Default | Meaning |
|--------|------|---------|---------|
| `--dest HOST:PORT` | string, repeatable | *(required)* | A destination. Repeat for multiple targets. |
| `--rounds N` | int | `50` | Number of probes sent to each destination. |
| `--payload-bytes N` | int | `64` | Datagram size in bytes (clamped up to the header size). |
| `--timeout-s S` | float | `0.5` | Per-round receive/loss cutoff in seconds. |
| `--interval-s S` | float | `0.0` | Pause between rounds in seconds. |
| `--sla-threshold-ms MS` | float | *(none)* | If set, also produce an SLA report against this threshold. |
| `--sla-percentile P` | float | `99.0` | Percentile at which each destination is judged for the SLA. |
| `--json` | flag | off | Emit machine-readable JSON instead of the text summary. |

### Human-readable output

```text
127.0.0.1:9873: p50=0.0558 p95=0.2865 p99=0.3331 jitter=0.0808 loss=0.00% (n=200)

Synchro SLA report (measured, not guaranteed)
  threshold           : 1.000 ms
  evaluated at        : p99
  destinations        : 1/1 meeting threshold (100.00%)
  samples within thr. : 200/200 (100.00%)
```

### JSON output

With `--json`, the tool prints an object with a `stats` map (per destination)
and, when `--sla-threshold-ms` is given, an `sla` object:

```json
{
  "stats": {
    "127.0.0.1:9873": {
      "dest": "127.0.0.1:9873",
      "sent": 200, "acked": 200, "lost": 0,
      "delivery_rate": 1.0, "loss_rate": 0.0, "samples": 200,
      "min_ms": 0.0421, "p50_ms": 0.0558, "p95_ms": 0.2865,
      "p99_ms": 0.3331, "max_ms": 0.4012, "mean_ms": 0.0771,
      "jitter_ms": 0.0808
    }
  },
  "sla": {
    "threshold_ms": 1.0, "percentile": 99.0,
    "total_dests": 1, "dests_meeting": 1, "dest_compliance": 1.0,
    "total_samples": 200, "samples_within_threshold": 200,
    "sample_compliance": 1.0,
    "per_dest": { "127.0.0.1:9873": { "meets_threshold": true, "percentile_value_ms": 0.3331 } }
  }
}
```

(Numbers above are illustrative of the format; real values come from your run.)

## Self-contained loopback demo

Most hosts will not echo an arbitrary UDP probe, so they show up honestly as
loss. To see real, non-loss numbers, run the built-in echo server first:

```bash
# Terminal 1 — loopback echo server on port 9999
SYNCHRO_ECHO_MAIN=1 SYNCHRO_ECHO_PORT=9999 python -m synchro.dispatcher

# Terminal 2 — measure 200 rounds against a 1 ms p99 target
python -m synchro.cli udp --dest 127.0.0.1:9999 --rounds 200 \
    --interval-s 0.001 --sla-threshold-ms 1.0 --sla-percentile 99
```

Echo-server environment variables:

| Variable | Default | Meaning |
|----------|---------|---------|
| `SYNCHRO_ECHO_MAIN` | *(unset)* | When set, running `python -m synchro.dispatcher` starts the echo server. |
| `SYNCHRO_ECHO_HOST` | `127.0.0.1` | Bind address. |
| `SYNCHRO_ECHO_PORT` | `9999` | Bind port. |

## Exit codes

- `0` — the `udp` command completed and printed results.
- Non-zero — argument parsing failed or an unexpected error occurred.

Note that recorded packet loss is **not** an error — it is valid measured data
and still exits `0`.
