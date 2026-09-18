# Ubuntu White Disk and Per-Sense Health

## State-of-the-art reference

Windows provides a useful vocabulary for this design. `Get-PhysicalDisk` exposes a disk `HealthStatus` with Healthy, Warning, Unhealthy, and Unknown states. Windows `Get-StorageReliabilityCounter` exposes device reliability information including temperature, read/write errors, wear, power-on hours, and latency counters. Microsoft documents these as storage reliability counters reported by the storage device. citeturn0search0turn0search1turn0search2

Ubuntu White adopts the **concept**, not the Windows API. The kernel layer should obtain the best available device health information from the platform's storage interface (SMART/ATA, NVMe health data, SCSI/SAS health mechanisms, or another appropriate driver interface).

## Two distinct health levels

### Device health

Describes the physical storage device or storage path:

- temperature;
- power-on hours;
- read errors;
- write errors;
- wear where the device exposes it;
- maximum observed read/write/flush latency;
- sampling time;
- overall state: `unknown`, `healthy`, `warning`, `unhealthy`.

### File-Sense health

Describes an individual physical copy of a logical file:

- file identity;
- Sense/copy number 1–3;
- content verification state;
- metadata verification state;
- device-health state at sampling time;
- blocks checked;
- read errors observed while checking;
- checksum failures;
- sampling time.

This distinction is essential: a disk can be healthy while one file copy is corrupt, and a file can verify correctly while the underlying device is reporting warning conditions.

## SMART terminology

S.M.A.R.T. is treated as a **device evidence source**, not as a universal health score. Different devices expose different attributes and thresholds. The Ubuntu White health layer therefore stores normalized fields plus an opaque source/vendor record where necessary.

A normalized `healthy` state means the available evidence does not currently indicate a problem; it does not guarantee future reliability.

## Per-Sense relationship

For each logical file:

```text
Logical File
  ├── Sense/Copy 1 ── File-Sense Health ── Device Health
  ├── Sense/Copy 2 ── File-Sense Health ── Device Health
  └── Sense/Copy 3 ── File-Sense Health ── Device Health
```

The same physical device may host multiple copies. Consequently, copy health is not simply a copy of the device health value.

## Kernel boundary

The initial C module defines the normalized structures only. It does not yet issue vendor-specific SMART commands from filesystem code. Production collection should occur through the appropriate kernel storage/driver interfaces and expose a stable normalized record to the Ubuntu White filesystem layer.

Filesystem code should not guess device health from file timestamps, names, or generic ratings.

## Integration with existing tools

- `comb` — collect and compare device and per-copy health.
- `lf` — display health with `-H` and detailed disk health with an explicit flag.
- `mf` — must not permit arbitrary fabrication of device-health evidence.
- `drm` — may use health evidence to inform an authorized copy-removal plan, but health alone must never authorize destructive deletion.
