# Ubuntu White Disk Health Monitor

The disk-health monitor is a userland service/profile for collecting SMART, NVMe, SCSI, and operating-system storage reliability evidence and associating observations with Ubuntu White physical file copies.

## Names

Canonical executable: `disk-health-monitor`

Systemd unit: `disk-health-monitor.service`

Cron compatibility profile: `disk-health-monitor.cron`

## Execution modes

Systemd is the preferred continuous mode. Cron is an alternative scheduled mode. Installations should normally select one scheduling mechanism to avoid duplicate samples.

## Privilege policy

The project uses a conceptual Ubuntu White privilege level of 3+ for operations requiring privileged device or filesystem inspection. Linux does not natively define numbered sudo levels; deployment maps this project policy to ordinary Linux users, groups, capabilities, and sudo rules.

## Safety

The monitor is observational. Health evidence must not by itself authorize modification or deletion of files. The executable and configuration should be protected by the Negamane integrity model.
