/* SPDX-License-Identifier: GPL-2.0 */
/*
 * white_installer_orchestrator.c — the White Edition "smooth" install
 * orchestrator ELF, the usual/default front door for installing the system.
 *
 * WHAT IT IS:
 *   A small, portable C11 native binary that provides the full guided install
 *   experience end to end: host/capability PROBE -> non-destructive desktop
 *   PREVIEW -> COMPONENT selection (checkbox + CLI parity) -> disk/TARGET
 *   selection -> explicit CONFIRM -> DELEGATE to the proven Bash install
 *   engine -> emit an AUDIT report.
 *
 * WHAT IT IS NOT:
 *   It contains NO low-level system-provisioning logic of its own (no disk
 *   layout, no filesystem creation, no root-jail entry, no package fetching).
 *   Every privileged operation is performed by the existing Bash engine
 *   scripts/galactic-cherry-installer, which the orchestrator resolves and
 *   hands control to via execv. The orchestrator itself runs unprivileged;
 *   the engine enforces root on its own. If the engine is missing the
 *   orchestrator degrades gracefully to the existing native entry script.
 *
 *   The legacy path stays fully usable: scripts/galactic-cherry-installer can
 *   still be run directly, and desktop_install_probe is untouched.
 *
 * Copyright (C) 2026 MEARVK LLC
 * Author: Maximilian Eric Alexander Rupplin von Keffikon
 */
#define _GNU_SOURCE
#include "white_installer_orchestrator.h"
#include "desktop_install_probe.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* ------------------------------------------------------------------ */
/* Single source of truth for the optional component contract.        */
/* Mirrors the Bash engine's parallel arrays exactly.                 */
/* ------------------------------------------------------------------ */
static const char *const COMPONENT_IDS[WIO_COMPONENT_COUNT] = {
    "ubuntu-white",
    "security",
    "git-improved",
    "jwstf",
};

static const char *const COMPONENT_DESCS[WIO_COMPONENT_COUNT] = {
    "Ubuntu White theme + icon overlay (GNOME)",
    "OS security suite (ClamAV, UFW, AppArmor, fail2ban, unattended-upgrades, rkhunter, chkrootkit)",
    "Improved Git (modern git + git-lfs + companions)",
    "JWSTF / NitroWebExpress Java web server",
};

/* 1 = ON by default, 0 = OFF by default. */
static const int COMPONENT_DEFAULTS[WIO_COMPONENT_COUNT] = { 1, 1, 1, 0 };

/* ------------------------------------------------------------------ */
/* Resolved run state.                                                */
/* ------------------------------------------------------------------ */
typedef struct {
    int selected[WIO_COMPONENT_COUNT]; /* current ON/OFF per component      */
    int preset;                        /* a selection was supplied non-interactively */
    char desktop[16];                  /* mate | gnome | vanilla (may be empty) */
    char target[PATH_MAX];             /* chosen disk/target path (may be empty) */
    int non_interactive;               /* --non-interactive                 */
    int delegate;                      /* affirmative gate to actually install */
} run_state;

/* ------------------------------------------------------------------ */
/* Small helpers.                                                     */
/* ------------------------------------------------------------------ */
static int path_exists(const char *p) { struct stat s; return stat(p, &s) == 0; }
static int is_dir(const char *p) { struct stat s; return stat(p, &s) == 0 && S_ISDIR(s.st_mode); }

static int component_index(const char *id) {
    for (int i = 0; i < WIO_COMPONENT_COUNT; i++)
        if (strcmp(id, COMPONENT_IDS[i]) == 0) return i;
    return -1;
}

static int join_path(char *out, size_t n, const char *base, const char *leaf) {
    int written = snprintf(out, n, "%s/%s", base, leaf);
    return written >= 0 && (size_t)written < n ? 0 : -1;
}

/*
 * Locate the Git clone exactly like desktop_install_probe does:
 *   1. cwd is a clone (has .git)
 *   2. ./Ubuntu.Determinant.Beta.Restricted/.git
 *   3. $HOME/DIP_DEFAULT_CLONE_SUBPATH
 */
static int locate_repo(char *out, size_t n) {
    const char *home = getenv("HOME");
    char p[PATH_MAX];
    char resolved[PATH_MAX];

    if (is_dir(".git") && realpath(".", resolved)) {
        if (strlen(resolved) >= n) return -1;
        strcpy(out, resolved);
        return 0;
    }
    if (is_dir("Ubuntu.Determinant.Beta.Restricted/.git") &&
        realpath("Ubuntu.Determinant.Beta.Restricted", resolved)) {
        if (strlen(resolved) >= n) return -1;
        strcpy(out, resolved);
        return 0;
    }
    if (home) {
        if (snprintf(p, sizeof p, "%s/%s", home, DIP_DEFAULT_CLONE_SUBPATH) >= (int)sizeof p)
            return -1;
        if (is_dir(p) && realpath(p, resolved)) {
            if (strlen(resolved) >= n) return -1;
            strcpy(out, resolved);
            return 0;
        }
    }
    return -1;
}

/* Return non-zero if a tool is discoverable on PATH (via command -v). */
static int have_tool(const char *tool) {
    char cmd[256];
    if (snprintf(cmd, sizeof cmd, "command -v %s >/dev/null 2>&1", tool) >= (int)sizeof cmd)
        return 0;
    return system(cmd) == 0;
}

/* Build a normalized space-separated selection string from run_state. */
static void selection_string(const run_state *st, char *out, size_t n) {
    out[0] = '\0';
    size_t used = 0;
    for (int i = 0; i < WIO_COMPONENT_COUNT; i++) {
        if (!st->selected[i]) continue;
        int w = snprintf(out + used, n - used, "%s%s",
                         used ? " " : "", COMPONENT_IDS[i]);
        if (w < 0 || (size_t)w >= n - used) break;
        used += (size_t)w;
    }
}

/* ------------------------------------------------------------------ */
/* Component list parsing (comma/space separated), --enable/--disable. */
/* ------------------------------------------------------------------ */
static void apply_component_list(run_state *st, const char *list, int on) {
    char buf[512];
    if (snprintf(buf, sizeof buf, "%s", list) >= (int)sizeof buf) return;
    for (char *p = buf; *p; p++)
        if (*p == ',') *p = ' ';
    char *save = NULL;
    for (char *tok = strtok_r(buf, " \t", &save); tok; tok = strtok_r(NULL, " \t", &save)) {
        int idx = component_index(tok);
        if (idx >= 0) {
            st->selected[idx] = on;
        } else {
            fprintf(stderr, "[warn] unknown component ignored: %s\n", tok);
        }
    }
    st->preset = 1;
}

/* ------------------------------------------------------------------ */
/* Usage / help.                                                      */
/* ------------------------------------------------------------------ */
static void usage(void) {
    printf("%s %s — %s\n\n", WIO_PROGRAM, WIO_VERSION, WIO_EDITION);
    printf("Usage: %s [OPTIONS]\n\n", WIO_PROGRAM);
    puts("The usual, guided \"smooth\" front door for installing the system.");
    puts("It probes the host, previews the desktop (read-only), lets you pick");
    puts("components and a target, asks for explicit confirmation, then hands");
    puts("off to the proven Bash install engine (" WIO_ENGINE_NAME ").");
    puts("");
    puts("With no delegating flag it performs a DRY RUN (probe + preview + plan");
    puts("+ audit) and never installs anything.");
    puts("");
    puts("Options:");
    puts("  --desktop <mate|gnome|vanilla>   Select the desktop environment.");
    puts("  --enable <comma,list>            Enable optional components.");
    puts("  --disable <comma,list>           Disable optional components.");
    puts("  --target, --disk <path>          Target device path (passed through).");
    puts("  --non-interactive                Skip prompts; use provided/default selections.");
    puts("  --install, --confirm             Authorize delegation to the engine.");
    puts("  --dry-run                        Plan only, never delegate (this is the default).");
    puts("  --help, -h                       Show this help and exit.");
    puts("");
    puts("Selectable components (--enable / --disable, or INSTALL_COMPONENTS env):");
    for (int i = 0; i < WIO_COMPONENT_COUNT; i++) {
        printf("  %-14s %s (default %s)\n",
               COMPONENT_IDS[i], COMPONENT_DESCS[i],
               COMPONENT_DEFAULTS[i] ? "ON" : "OFF");
    }
    puts("");
    puts("Environment variables (honored like the Bash engine):");
    puts("  INSTALL_DESKTOP       mate | gnome | vanilla");
    puts("  INSTALL_COMPONENTS    space- or comma-separated component list");
    puts("  GC_COMPONENTS         alias for INSTALL_COMPONENTS");
    puts("");
    puts("Delegation scheme: the orchestrator passes the resolved selection to");
    puts("the engine via INSTALL_COMPONENTS in the child environment plus");
    puts("--non-interactive and --desktop, so the guided/headless run is smooth.");
    puts("The orchestrator runs unprivileged; the engine enforces root itself.");
}

/* ------------------------------------------------------------------ */
/* Environment defaults (INSTALL_DESKTOP / INSTALL_COMPONENTS / GC_*). */
/* ------------------------------------------------------------------ */
static void seed_from_env(run_state *st) {
    const char *d = getenv("INSTALL_DESKTOP");
    if (d && *d) snprintf(st->desktop, sizeof st->desktop, "%s", d);

    const char *comps = getenv("INSTALL_COMPONENTS");
    if (!comps || !*comps) comps = getenv("GC_COMPONENTS");
    if (comps && *comps) {
        for (int i = 0; i < WIO_COMPONENT_COUNT; i++) st->selected[i] = 0;
        apply_component_list(st, comps, 1);
    }
}

/* ------------------------------------------------------------------ */
/* Argument parsing. Accepts both `--flag value` and `--flag=value`.  */
/* Returns 0 on success, negative on error, positive to exit-0 (help).*/
/* ------------------------------------------------------------------ */
static int need_value(const char *flag, const char *val) {
    if (!val) {
        fprintf(stderr, "ERROR: %s requires a value.\n", flag);
        fprintf(stderr, "Try '%s --help' for usage.\n", WIO_PROGRAM);
        return -1;
    }
    return 0;
}

static int parse_args(int argc, char **argv, run_state *st) {
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        char *eq = strchr(arg, '=');
        const char *inlineval = NULL;
        char name[64];
        if (eq && strncmp(arg, "--", 2) == 0) {
            size_t len = (size_t)(eq - arg);
            if (len >= sizeof name) len = sizeof name - 1;
            memcpy(name, arg, len);
            name[len] = '\0';
            inlineval = eq + 1;
            arg = name;
        }

        if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
            usage();
            return 1;
        } else if (strcmp(arg, "--desktop") == 0) {
            const char *v = inlineval ? inlineval : (++i < argc ? argv[i] : NULL);
            if (need_value("--desktop", v) != 0) return -1;
            snprintf(st->desktop, sizeof st->desktop, "%s", v);
        } else if (strcmp(arg, "--enable") == 0) {
            const char *v = inlineval ? inlineval : (++i < argc ? argv[i] : NULL);
            if (need_value("--enable", v) != 0) return -1;
            apply_component_list(st, v, 1);
        } else if (strcmp(arg, "--disable") == 0) {
            const char *v = inlineval ? inlineval : (++i < argc ? argv[i] : NULL);
            if (need_value("--disable", v) != 0) return -1;
            apply_component_list(st, v, 0);
        } else if (strcmp(arg, "--target") == 0 || strcmp(arg, "--disk") == 0) {
            const char *v = inlineval ? inlineval : (++i < argc ? argv[i] : NULL);
            if (need_value(arg, v) != 0) return -1;
            snprintf(st->target, sizeof st->target, "%s", v);
        } else if (strcmp(arg, "--non-interactive") == 0) {
            st->non_interactive = 1;
        } else if (strcmp(arg, "--install") == 0 || strcmp(arg, "--confirm") == 0) {
            st->delegate = 1;
        } else if (strcmp(arg, "--dry-run") == 0) {
            st->delegate = 0;
        } else {
            fprintf(stderr, "ERROR: Unknown option: %s\n", argv[i]);
            fprintf(stderr, "Try '%s --help' for usage.\n", WIO_PROGRAM);
            return -1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Stage (1): PROBE.                                                  */
/* ------------------------------------------------------------------ */
static void stage_probe(const char *repo, const char *engine, int engine_present,
                        int have_whiptail, int have_dialog) {
    puts("");
    puts("== Stage 1/7: PROBE (host + capabilities) ==");
    printf("[ok] Git clone: %s\n", repo);

    FILE *fp = popen("uname -s -r -m 2>/dev/null", "r");
    if (fp) {
        char line[256];
        if (fgets(line, sizeof line, fp)) {
            size_t l = strlen(line);
            if (l && line[l - 1] == '\n') line[l - 1] = '\0';
            printf("[ok] Host: %s\n", line);
        }
        pclose(fp);
    }
    printf("[%s] Bash install engine: %s\n",
           engine_present ? "ok" : "warn", engine);
    printf("[info] TUI frontend: whiptail=%s dialog=%s (a frontend is optional here)\n",
           have_whiptail ? "yes" : "no", have_dialog ? "yes" : "no");
    puts("[info] running unprivileged; no root required to probe or preview.");
}

/* ------------------------------------------------------------------ */
/* Stage (2): PREVIEW (non-destructive, read-only).                   */
/* ------------------------------------------------------------------ */
static int stage_preview(const char *repo) {
    char preview[PATH_MAX];
    puts("");
    puts("== Stage 2/7: PREVIEW (non-destructive desktop preview) ==");
    if (join_path(preview, sizeof preview, repo, DIP_PREVIEW_DOCUMENT) != 0) {
        fputs("[warn] preview document path is too long.\n", stderr);
        return 0;
    }
    if (path_exists(preview))
        printf("[ok] Desktop preview (read-only): %s\n", preview);
    else
        printf("[warn] preview document not found: %s\n", preview);
    puts("[info] preview is read-only; nothing on the host is altered.");
    return path_exists(preview);
}

/* ------------------------------------------------------------------ */
/* Stage (3): COMPONENTS.                                             */
/* ------------------------------------------------------------------ */
static void print_component_list(const run_state *st) {
    for (int i = 0; i < WIO_COMPONENT_COUNT; i++) {
        printf("  %d) [%s] %-14s %s\n",
               i + 1, st->selected[i] ? "x" : " ",
               COMPONENT_IDS[i], COMPONENT_DESCS[i]);
    }
}

static void stage_components(run_state *st) {
    puts("");
    puts("== Stage 3/7: COMPONENTS ==");

    int interactive = !st->non_interactive && !st->preset && isatty(STDIN_FILENO);

    if (!interactive) {
        /* Headless / preset: if nothing selected, fall back to defaults. */
        int any = 0;
        for (int i = 0; i < WIO_COMPONENT_COUNT; i++) any |= st->selected[i];
        if (!any) {
            run_state tmp = *st;
            for (int i = 0; i < WIO_COMPONENT_COUNT; i++) tmp.selected[i] = 0;
            apply_component_list(&tmp, WIO_DEFAULT_COMPONENTS, 1);
            for (int i = 0; i < WIO_COMPONENT_COUNT; i++) st->selected[i] = tmp.selected[i];
        }
        print_component_list(st);
        puts("[info] using preset/non-interactive selection.");
        return;
    }

    for (;;) {
        print_component_list(st);
        puts("");
        puts("  Enter comma-separated numbers to TOGGLE (e.g. 1,4),");
        puts("  or press Enter to accept the selection shown above.");
        printf("  Toggle: ");
        fflush(stdout);

        char line[128];
        if (!fgets(line, sizeof line, stdin)) break;
        /* strip newline */
        size_t l = strlen(line);
        if (l && line[l - 1] == '\n') line[--l] = '\0';
        if (l == 0) break; /* accept */

        for (char *p = line; *p; p++)
            if (*p == ',') *p = ' ';
        char *save = NULL;
        for (char *tok = strtok_r(line, " \t", &save); tok; tok = strtok_r(NULL, " \t", &save)) {
            int num = atoi(tok);
            if (num >= 1 && num <= WIO_COMPONENT_COUNT)
                st->selected[num - 1] = !st->selected[num - 1];
        }
        puts("");
    }
}

/* ------------------------------------------------------------------ */
/* Stage (4): TARGET (record only, never touch the device).          */
/* ------------------------------------------------------------------ */
static void stage_target(run_state *st) {
    puts("");
    puts("== Stage 4/7: TARGET ==");
    if (st->target[0]) {
        printf("[ok] Target recorded (not touched): %s\n", st->target);
        return;
    }

    if (st->delegate && !st->non_interactive && isatty(STDIN_FILENO)) {
        printf("  Target device path (e.g. /dev/sda), or Enter to leave to the engine: ");
        fflush(stdout);
        char line[PATH_MAX];
        if (fgets(line, sizeof line, stdin)) {
            size_t l = strlen(line);
            if (l && line[l - 1] == '\n') line[--l] = '\0';
            if (l) snprintf(st->target, sizeof st->target, "%s", line);
        }
    }

    if (st->target[0])
        printf("[ok] Target recorded (not touched): %s\n", st->target);
    else
        puts("[info] no target specified; the engine's own disk step will handle selection.");
}

/* ------------------------------------------------------------------ */
/* AUDIT report (ARCHITECTURE.md section 7 fields, no secrets/PII).   */
/* ------------------------------------------------------------------ */

/*
 * Outcome of the run, so the audit reports what actually happened rather than
 * what was merely planned. The audit's operation/verification/warnings lines
 * are derived from this, never from st->delegate alone.
 */
typedef enum {
    WIO_OUTCOME_DRYRUN = 0,   /* no delegating flag: plan/preview only        */
    WIO_OUTCOME_ABORTED,      /* --install but confirmation declined/refused  */
    WIO_OUTCOME_DELEGATED,    /* authorized and about to hand off to the engine */
} wio_outcome;

static void iso_time(char *out, size_t n) {
    time_t t = time(NULL);
    struct tm g;
    gmtime_r(&t, &g);
    strftime(out, n, "%Y-%m-%dT%H:%M:%SZ", &g);
}

static void stage_audit(const run_state *st, const char *repo, const char *engine,
                        const char *engine_cmdline, const char *start_iso,
                        int preview_ok, int engine_present, wio_outcome outcome) {
    char end_iso[32];
    iso_time(end_iso, sizeof end_iso);

    char comps[256];
    selection_string(st, comps, sizeof comps);

    puts("");
    puts("== Stage 7/7: AUDIT REPORT ==");
    puts("----------------------------------------------------------------");
    printf("installer version      : %s %s\n", WIO_PROGRAM, WIO_VERSION);

    char host[256] = "unknown";
    FILE *fp = popen("uname -s -r -m 2>/dev/null", "r");
    if (fp) {
        if (fgets(host, sizeof host, fp)) {
            size_t l = strlen(host);
            if (l && host[l - 1] == '\n') host[l - 1] = '\0';
        }
        pclose(fp);
    }
    printf("host                   : %s\n", host);
    const char *op_label;
    switch (outcome) {
        case WIO_OUTCOME_DELEGATED: op_label = "install (delegated to engine)"; break;
        case WIO_OUTCOME_ABORTED:   op_label = "install (authorized but NOT delegated; confirmation refused)"; break;
        default:                    op_label = "dry-run (plan/preview only)"; break;
    }
    printf("operation              : %s\n", op_label);
    printf("source                 : %s (%s)\n", repo, DIP_REPOSITORY_URL);
    printf("resolved target        : %s\n", st->target[0] ? st->target : "(deferred to engine disk step)");
    printf("resolved desktop       : %s\n", st->desktop[0] ? st->desktop : "(engine default)");
    printf("resolved components    : %s\n", comps[0] ? comps : "(none)");
    printf("commands/contracts     : %s\n", engine_cmdline);
    if (outcome == WIO_OUTCOME_DELEGATED) {
        printf("privilege boundary     : orchestrator ran UNPRIVILEGED; privileged\n");
        printf("                         operations delegated to the root-enforcing engine (%s)\n",
               engine_present ? engine : "engine absent -> native fallback");
    } else {
        printf("privilege boundary     : orchestrator ran UNPRIVILEGED; no privileged\n");
        printf("                         operation was delegated (nothing was executed)\n");
    }
    printf("start/end time         : %s / %s\n", start_iso, end_iso);
    switch (outcome) {
        case WIO_OUTCOME_DELEGATED:
            printf("verification result    : probe ok; preview %s; plan built ok; delegating to engine\n",
                   preview_ok ? "ok" : "missing");
            printf("warnings               : target device data will be erased by the engine\n");
            break;
        case WIO_OUTCOME_ABORTED:
            printf("verification result    : probe ok; preview %s; plan built ok; delegation refused (no changes)\n",
                   preview_ok ? "ok" : "missing");
            printf("warnings               : install was authorized on the CLI but confirmation was refused; nothing was delegated\n");
            break;
        default:
            printf("verification result    : probe ok; preview %s; plan built ok (no changes)\n",
                   preview_ok ? "ok" : "missing");
            printf("warnings               : none (dry-run performed no changes)\n");
            break;
    }
    puts("no secrets/PII in this report (credentials are never collected here).");
    puts("----------------------------------------------------------------");
    puts("[info] a persisted copy may be written to $TMPDIR or a path you supply;");
    puts("       nothing is written outside the repo/tmp by default.");
}

/* ------------------------------------------------------------------ */
/* Build a human-readable representation of the engine command line   */
/* used both for the confirm summary and the audit report.           */
/* ------------------------------------------------------------------ */
#define WIO_CMDLINE_MAX (PATH_MAX + 512)

static void build_engine_cmdline(const run_state *st, const char *engine, char *out, size_t n) {
    char comps[256];
    char desk[32];
    selection_string(st, comps, sizeof comps);
    /* Copy the desktop into a fixed, compiler-visible bound so the format is
     * provably free of truncation. */
    desk[0] = '\0';
    if (st->desktop[0])
        snprintf(desk, sizeof desk, " --desktop %s", st->desktop);
    snprintf(out, n, "INSTALL_COMPONENTS='%s' %s%s --non-interactive",
             comps, engine, desk);
}

/* ------------------------------------------------------------------ */
/* Stage (5)+(6): CONFIRM then DELEGATE.                              */
/* Returns an exit code if it does NOT delegate; on delegation the    */
/* process image is replaced (execv) and control does not return.     */
/* ------------------------------------------------------------------ */
/*
 * Return non-zero only if `line` (a fgets result) is exactly an affirmative
 * "yes"/"y" once its trailing newline and surrounding blanks are stripped.
 * A loose prefix match would wrongly accept "yesterday" as authorization for
 * a disk-erasing install, so the comparison is exact.
 */
static int is_affirmative(const char *line) {
    if (!line) return 0;
    char buf[16];
    snprintf(buf, sizeof buf, "%s", line);
    /* strip a single trailing newline / carriage return */
    size_t l = strlen(buf);
    while (l && (buf[l - 1] == '\n' || buf[l - 1] == '\r')) buf[--l] = '\0';
    /* strip leading blanks */
    char *s = buf;
    while (*s == ' ' || *s == '\t') s++;
    /* strip trailing blanks */
    l = strlen(s);
    while (l && (s[l - 1] == ' ' || s[l - 1] == '\t')) s[--l] = '\0';
    return strcmp(s, "yes") == 0 || strcmp(s, "y") == 0;
}

static int stage_confirm_and_delegate(run_state *st, const char *repo,
                                       const char *engine, int engine_present,
                                       const char *engine_cmdline,
                                       const char *start_iso, int preview_ok) {
    char comps[256];
    selection_string(st, comps, sizeof comps);

    puts("");
    puts("== Stage 5/7: CONFIRM ==");
    printf("  Installer : %s %s\n", WIO_PROGRAM, WIO_VERSION);
    printf("  Desktop   : %s\n", st->desktop[0] ? st->desktop : "(engine default)");
    printf("  Components : %s\n", comps[0] ? comps : "(defaults)");
    printf("  Target    : %s\n", st->target[0] ? st->target : "(engine disk step)");
    printf("  Engine    : %s\n", engine_cmdline);

    if (!st->delegate) {
        puts("[dry-run] plan complete; NOT delegating. Re-run with --install to proceed.");
        stage_audit(st, repo, engine, engine_cmdline, start_iso, preview_ok,
                    engine_present, WIO_OUTCOME_DRYRUN);
        return 0;
    }

    puts("  WARNING: proceeding will let the engine ERASE the target device.");
    if (st->target[0]) {
        /* The engine has no headless disk intake: it reads INSTALL_DISK only
         * from its own interactive read/dialog. A recorded --target is audited
         * but not forwarded, so a delegated headless run still stops at the
         * engine's disk step. Be honest about that here. */
        puts("  NOTE: the recorded target is not forwarded; the engine's own disk");
        puts("        step will still prompt for the device to erase.");
    }

    if (st->non_interactive) {
        /* Authorized only when selections were supplied explicitly. */
        if (!st->preset && !st->desktop[0] && !st->target[0]) {
            fputs("[refuse] --non-interactive --install without any explicit selection.\n", stderr);
            fputs("         Provide --desktop/--enable/--target (or INSTALL_COMPONENTS) to authorize.\n", stderr);
            stage_audit(st, repo, engine, engine_cmdline, start_iso, preview_ok,
                        engine_present, WIO_OUTCOME_ABORTED);
            return 2;
        }
    } else if (isatty(STDIN_FILENO)) {
        printf("  Type 'yes' to authorize the install: ");
        fflush(stdout);
        char line[16];
        if (!fgets(line, sizeof line, stdin) || !is_affirmative(line)) {
            puts("[abort] not confirmed; nothing was delegated.");
            stage_audit(st, repo, engine, engine_cmdline, start_iso, preview_ok,
                        engine_present, WIO_OUTCOME_ABORTED);
            return 0;
        }
    } else {
        fputs("[refuse] no TTY for confirmation; use --non-interactive with explicit selections.\n", stderr);
        stage_audit(st, repo, engine, engine_cmdline, start_iso, preview_ok,
                    engine_present, WIO_OUTCOME_ABORTED);
        return 2;
    }

    /* Delegation is authorized and will now proceed. Emit the audit BEFORE the
     * handoff because execv replaces this process and would discard it. The
     * record is only written on this path, so it reflects a real delegation. */
    stage_audit(st, repo, engine, engine_cmdline, start_iso, preview_ok,
                engine_present, WIO_OUTCOME_DELEGATED);

    puts("");
    puts("== Stage 6/7: DELEGATE ==");

    /* Set the child environment: pass the resolved selection via
     * INSTALL_COMPONENTS and let the engine run headless. We never touch
     * root ourselves; the engine enforces privilege on its own. */
    setenv("INSTALL_COMPONENTS", comps, 1);
    if (st->desktop[0]) setenv("INSTALL_DESKTOP", st->desktop, 1);

    if (engine_present) {
        char *argvv[8];
        int a = 0;
        argvv[a++] = (char *)engine;
        argvv[a++] = "--non-interactive";
        if (st->desktop[0]) {
            argvv[a++] = "--desktop";
            argvv[a++] = st->desktop;
        }
        argvv[a] = NULL;
        printf("[delegate] handing off to %s\n", engine);
        execv(engine, argvv);
        perror("execv");
        return 6;
    }

    /* Graceful fallback: the engine is absent, run the native entry script. */
    {
        char native[PATH_MAX];
        if (join_path(native, sizeof native, repo, DIP_NATIVE_INSTALLER) == 0 &&
            path_exists(native)) {
            printf("[delegate] engine absent; falling back to %s\n", native);
            char *argvv[3];
            argvv[0] = (char *)"sh";
            argvv[1] = native;
            argvv[2] = NULL;
            execv("/bin/sh", argvv);
            perror("execv");
            return 6;
        }
    }
    fputs("[error] neither the Bash engine nor the native installer is available.\n", stderr);
    return 5;
}

/* ------------------------------------------------------------------ */
/* main                                                               */
/* ------------------------------------------------------------------ */
int main(int argc, char **argv) {
    run_state st;
    memset(&st, 0, sizeof st);
    for (int i = 0; i < WIO_COMPONENT_COUNT; i++)
        st.selected[i] = COMPONENT_DEFAULTS[i];

    char start_iso[32];
    iso_time(start_iso, sizeof start_iso);

    seed_from_env(&st);

    int pr = parse_args(argc, argv, &st);
    if (pr > 0) return 0;   /* --help printed */
    if (pr < 0) return 2;   /* bad args       */

    printf("%s %s — %s\n", WIO_PROGRAM, WIO_VERSION, WIO_EDITION);
    puts("The usual smooth installer. Default action is a safe DRY RUN.");

    /* Locate the repository the same way the probe does. */
    char repo[PATH_MAX];
    if (locate_repo(repo, sizeof repo) != 0) {
        fputs("[error] could not locate the repository clone (no .git found).\n", stderr);
        return 3;
    }

    char engine[PATH_MAX];
    if (join_path(engine, sizeof engine, repo, WIO_ENGINE_RELPATH) != 0) {
        fputs("[error] engine path is too long.\n", stderr);
        return 4;
    }
    int engine_present = path_exists(engine);
    int have_whiptail = have_tool("whiptail");
    int have_dialog = have_tool("dialog");

    stage_probe(repo, engine, engine_present, have_whiptail, have_dialog);
    int preview_ok = stage_preview(repo);
    stage_components(&st);
    stage_target(&st);

    char engine_cmdline[WIO_CMDLINE_MAX];
    build_engine_cmdline(&st, engine, engine_cmdline, sizeof engine_cmdline);

    /*
     * The audit is emitted from inside stage_confirm_and_delegate so it always
     * reflects the actual outcome:
     *   - dry-run           -> audit labeled "dry-run (plan/preview only)"
     *   - authorized+delegated -> audit labeled "install (delegated to engine)",
     *                             printed BEFORE execv replaces this process
     *   - authorized+refused/aborted -> audit labeled "authorized but NOT
     *                             delegated", printed before the non-delegating
     *                             return so a refused run is never mislabeled.
     */
    return stage_confirm_and_delegate(&st, repo, engine, engine_present,
                                      engine_cmdline, start_iso, preview_ok);
}
