"""Command-line entry point for Synchro measurements.

Usage examples
--------------
Measure RTT to UDP destinations (they must echo the Synchro header to reply;
unreached hosts are reported honestly as loss):

    python -m synchro.cli udp --dest 127.0.0.1:9999 --rounds 100 \
        --sla-threshold-ms 1.0 --sla-percentile 99

Run the built-in loopback echo server in another shell first:

    SYNCHRO_ECHO_MAIN=1 SYNCHRO_ECHO_PORT=9999 python -m synchro.dispatcher
"""

from __future__ import annotations

import argparse
import json
import sys
from typing import List, Tuple

from .dispatcher import UdpDispatcher
from .sla import SlaReporter


def _parse_dests(items: List[str]) -> List[Tuple[str, int]]:
    out = []
    for it in items:
        host, _, port = it.rpartition(":")
        if not host or not port:
            raise SystemExit(f"invalid --dest {it!r}; expected host:port")
        out.append((host, int(port)))
    return out


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="synchro", description="Measured latency, not guaranteed.")
    sub = ap.add_subparsers(dest="cmd", required=True)

    u = sub.add_parser("udp", help="UDP RTT measurement")
    u.add_argument("--dest", action="append", required=True, help="host:port (repeatable)")
    u.add_argument("--rounds", type=int, default=50)
    u.add_argument("--payload-bytes", type=int, default=64)
    u.add_argument("--timeout-s", type=float, default=0.5)
    u.add_argument("--interval-s", type=float, default=0.0)
    u.add_argument("--sla-threshold-ms", type=float, default=None)
    u.add_argument("--sla-percentile", type=float, default=99.0)
    u.add_argument("--json", action="store_true")

    args = ap.parse_args(argv)

    if args.cmd == "udp":
        dests = _parse_dests(args.dest)
        disp = UdpDispatcher(
            dests,
            payload_bytes=args.payload_bytes,
            timeout_s=args.timeout_s,
        )
        result = disp.run(rounds=args.rounds, interval_s=args.interval_s)

        payload = {"stats": result.as_dict()}
        if args.sla_threshold_ms is not None:
            rep = SlaReporter(args.sla_threshold_ms, percentile=args.sla_percentile)
            report = rep.evaluate(result.stats)
            payload["sla"] = report.as_dict()

        if args.json:
            print(json.dumps(payload, indent=2))
        else:
            for dest, s in result.as_dict().items():
                print(f"{dest}: p50={s['p50_ms']} p95={s['p95_ms']} "
                      f"p99={s['p99_ms']} jitter={s['jitter_ms']} "
                      f"loss={s['loss_rate']*100:.2f}% (n={s['samples']})")
            if args.sla_threshold_ms is not None:
                print()
                print(report.summary())
        return 0

    return 1


if __name__ == "__main__":
    sys.exit(main())
