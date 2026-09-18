# Aptitude System Health

A calm, business-friendly health surface for Ubuntu and compatible Linux hosts.

The health experience intentionally uses a restrained sequence:

**Green → White → Green → Yellow → White**

- **Green** — healthy / verified
- **White** — informational / normal
- **Green** — ready / operating normally
- **Yellow** — attention recommended, but not necessarily failure
- **White** — explanation and next action

The dashboard should not turn ordinary warnings into emergencies. A yellow result means Aptitude has found something worth reviewing; it does not by itself establish damage, compromise, or failure.

## Health signals

The first health collector checks, when available:

1. operating-system identity;
2. kernel identity;
3. available RAM;
4. one-minute load;
5. filesystem capacity;
6. systemd availability and failed units;
7. recent high-priority journal activity;
8. Aptitude SHA-256 integrity status;
9. package-manager presence;
10. evidence-manifest availability.

`systemctl` is used for systemd state inspection where systemd is present, while `journalctl` is used for journal inspection. These are standard systemd interfaces for introspection and journal querying. citeturn0search3turn0search0

## User experience

```text
                 APTITUDE
            Ubuntu System Health

              GREEN
          SYSTEM HEALTHY

  Software       Verified
  Services       Normal
  Integrity      SHA-256 verified
  Memory         Comfortable
  Updates        Review available
  Evidence       1–2–3–4 available

       [ Review System ]

      Free • Forever • Local
```

The interface should remain usable without requiring the operator to understand systemd, hashes, cron, cgroups, package databases, or kernel internals. Detailed evidence remains one level deeper.

## Health result vocabulary

```text
HEALTHY       all required checks passed
NORMAL        observed state is ordinary or informational
ATTENTION     recommendation or non-critical deviation
UNKNOWN       a check could not be performed
UNAVAILABLE   the host does not provide the relevant surface
FAILED        a required health check failed
```

Unknown and unavailable must not be silently converted into healthy. Conversely, a missing optional subsystem must not be presented as a system failure.

## Free local experience

The base Aptitude health view is intended to be a **free, local, forever-available** experience. It does not require a subscription or remote service merely to inspect the local machine.

That product statement does not promise that optional future hosted services, commercial support, managed repositories, or other separately offered products will be free.
