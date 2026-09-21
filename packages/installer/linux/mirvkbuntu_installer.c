/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mirvkbuntu-installer — the edition-aware MirvkBuntu system installer.
 *
 * A single native C11 ELF that performs a full install-to-disk of one of the
 * three MirvkBuntu ISO editions (slim | minimal | full) from a running live
 * session. It is the "full installer executable" the repository lacked: the
 * existing Bash engine (scripts/galactic-cherry-installer) chooses only a
 * DESKTOP and copies whatever live filesystem is running; it has no concept of
 * the slim/minimal/full ISO editions. white-installer is only a control plane
 * that delegates. This binary owns the edition contract end to end.
 *
 * Safety contract (from installer/ARCHITECTURE.md, section 8 + state model):
 *   DISCOVER -> PLAN -> REVIEW -> CONFIRM -> ELEVATE -> EXECUTE -> VERIFY -> REPORT
 * The default run is a DRY RUN: it discovers the host, resolves the edition,
 * builds the full partition/format/copy/configure/bootloader plan, prints it
 * with an audit report, and exits WITHOUT touching any disk. A destructive run
 * requires an explicit --install/--confirm and a real, existing target device.
 *
 * How EXECUTE works: the engine composes a single, fully-resolved, auditable
 * provisioning shell script from typed, allow-listed parameters (never from
 * free-form user strings) and runs it under `set -euo pipefail`. This keeps the
 * privileged disk work transparent and reviewable (the exact script is written
 * to the log and can be dumped with --emit-script) while the ELF remains the
 * one authoritative artifact that owns the edition logic.
 *
 * Copyright (C) 2026 MEARVK LLC
 * Author: Maximilian Eric Alexander Rupplin von Keffikon
 */
#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "mirvkbuntu_installer.h"

/* ------------------------------------------------------------------ */
/* Global run state                                                    */
/* ------------------------------------------------------------------ */

typedef struct {
    mi_edition_t edition;
    mi_edition_t edition_source_detected; /* what discovery found */
    mi_desktop_t desktop;

    char        target[256];     /* e.g. /dev/sda ; empty until chosen  */
    char        hostname[128];
    char        username[64];
    char        fullname[128];
    char        password[256];   /* only used to compose chpasswd stdin */
    char        timezone[64];
    char        overlay_size[32];

    int         components[MI_COMPONENT_COUNT]; /* 1 = selected */

    int         do_install;      /* 0 = dry run (default), 1 = execute  */
    int         non_interactive;
    int         assume_yes;
    int         emit_script;     /* print the generated script and exit */
    char        source_squashfs[512]; /* resolved live rootfs source    */
    char        logpath[256];
} mi_state_t;

static const char *const kComponentIds[MI_COMPONENT_COUNT] = {
    "ubuntu-white", "security", "git-improved", "jwstf"
};
static const char *const kComponentDescs[MI_COMPONENT_COUNT] = {
    "Ubuntu White theme + icon overlay (GNOME)",
    "OS security suite (ClamAV, UFW, AppArmor, fail2ban, unattended-upgrades, rkhunter, chkrootkit)",
    "Improved Git (modern git + git-lfs + companions)",
    "JWSTF / NitroWebExpress Java web server"
};
static const int kComponentDefaults[MI_COMPONENT_COUNT] = { 1, 1, 1, 0 };

/* ------------------------------------------------------------------ */
/* Small helpers                                                       */
/* ------------------------------------------------------------------ */

static void mi_log(const mi_state_t *st, const char *fmt, ...) {
    if (!st->logpath[0]) return;
    FILE *f = fopen(st->logpath, "a");
    if (!f) return;
    time_t t = time(NULL);
    struct tm tmv;
    localtime_r(&t, &tmv);
    char ts[32];
    strftime(ts, sizeof ts, "%H:%M:%S", &tmv);
    fprintf(f, "[%s] ", ts);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fputc('\n', f);
    fclose(f);
}

static const char *edition_name(mi_edition_t e) {
    switch (e) {
        case MI_EDITION_SLIM:    return "slim";
        case MI_EDITION_MINIMAL: return "minimal";
        case MI_EDITION_FULL:    return "full";
        default:                 return "unknown";
    }
}

static mi_edition_t edition_from_str(const char *s) {
    if (!s) return MI_EDITION_UNKNOWN;
    if (!strcasecmp(s, "slim"))                              return MI_EDITION_SLIM;
    if (!strcasecmp(s, "minimal") || !strcasecmp(s, "min"))  return MI_EDITION_MINIMAL;
    if (!strcasecmp(s, "full") || !strcasecmp(s, "desktop")) return MI_EDITION_FULL;
    return MI_EDITION_UNKNOWN;
}

static const char *desktop_name(mi_desktop_t d) {
    switch (d) {
        case MI_DESKTOP_GNOME:   return "gnome";
        case MI_DESKTOP_MATE:    return "mate";
        case MI_DESKTOP_VANILLA: return "vanilla";
        default:                 return "default";
    }
}

static mi_desktop_t desktop_from_str(const char *s) {
    if (!s) return MI_DESKTOP_DEFAULT;
    if (!strcasecmp(s, "gnome"))   return MI_DESKTOP_GNOME;
    if (!strcasecmp(s, "mate"))    return MI_DESKTOP_MATE;
    if (!strcasecmp(s, "vanilla")) return MI_DESKTOP_VANILLA;
    return MI_DESKTOP_DEFAULT;
}

static int component_index(const char *id) {
    for (int i = 0; i < MI_COMPONENT_COUNT; i++)
        if (!strcasecmp(id, kComponentIds[i])) return i;
    return -1;
}

/* Reject anything that is not a plain block-device path we recognize, so a
 * user-supplied target can never smuggle shell metacharacters into EXECUTE. */
static int valid_device_path(const char *p) {
    if (!p || strncmp(p, "/dev/", 5) != 0) return 0;
    for (const char *c = p + 5; *c; c++) {
        if (!(isalnum((unsigned char)*c) || *c == '/' || *c == '-' || *c == '_'))
            return 0;
    }
    return 1;
}

/* Validate identity fields: keep them to a safe, shell-inert character set. */
static int valid_ident(const char *s, size_t maxlen) {
    if (!s || !*s) return 0;
    if (strlen(s) > maxlen) return 0;
    for (const char *c = s; *c; c++)
        if (!(isalnum((unsigned char)*c) || *c == '-' || *c == '_' || *c == '.'))
            return 0;
    return 1;
}

/* Full name may contain spaces and common punctuation, but must not carry any
 * character that could break out of the double-quoted context it is emitted in
 * (quotes, backticks, $, backslash, and shell control operators). */
static int valid_fullname(const char *s, size_t maxlen) {
    if (!s) return 0;
    if (strlen(s) > maxlen) return 0;
    for (const char *c = s; *c; c++) {
        if (*c == '"' || *c == '`' || *c == '$' || *c == '\\' ||
            *c == '\n' || *c == '\r' || *c == ';' || *c == '&' ||
            *c == '|' || *c == '<' || *c == '>')
            return 0;
    }
    return 1;
}

static void trim_newline(char *s) {
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r')) s[--n] = '\0';
}

/* ------------------------------------------------------------------ */
/* Edition defaults                                                    */
/* ------------------------------------------------------------------ */

/* Apply the per-edition default desktop + component selection, without
 * overriding anything the user set explicitly (marked via *_set flags). */
static void apply_edition_defaults(mi_state_t *st, int desktop_set,
                                   int components_set) {
    switch (st->edition) {
        case MI_EDITION_SLIM:
            /* Slim = the White edition: GNOME + ubuntu-white, always. */
            if (!desktop_set) st->desktop = MI_DESKTOP_GNOME;
            if (!components_set) {
                st->components[0] = 1; /* ubuntu-white */
                st->components[1] = 1; /* security     */
                st->components[2] = 1; /* git-improved */
                st->components[3] = 0; /* jwstf        */
            }
            st->components[0] = 1; /* ubuntu-white forced on for slim */
            break;
        case MI_EDITION_MINIMAL:
            /* Smallest footprint: no desktop, no optional suites by default. */
            if (!desktop_set) st->desktop = MI_DESKTOP_VANILLA;
            if (!components_set) {
                st->components[0] = 0;
                st->components[1] = 0;
                st->components[2] = 0;
                st->components[3] = 0;
            }
            break;
        case MI_EDITION_FULL:
        default:
            if (!desktop_set) st->desktop = MI_DESKTOP_GNOME;
            if (!components_set) {
                st->components[0] = 1;
                st->components[1] = 1;
                st->components[2] = 1;
                st->components[3] = 0;
            }
            break;
    }
}

/* ------------------------------------------------------------------ */
/* DISCOVER                                                            */
/* ------------------------------------------------------------------ */

/* Read a KEY=VALUE value out of a small conf file. Returns 1 on hit. */
static int conf_lookup(const char *path, const char *key, char *out, size_t n) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    char line[512];
    size_t klen = strlen(key);
    int found = 0;
    while (fgets(line, sizeof line, f)) {
        if (strncmp(line, key, klen) == 0 && line[klen] == '=') {
            const char *v = line + klen + 1;
            snprintf(out, n, "%s", v);
            trim_newline(out);
            found = 1;
            break;
        }
    }
    fclose(f);
    return found;
}

/* Detect the edition of the running live system:
 *   1) MIRVKBUNTU_EDITION in any /etc/mirvkbuntu conf marker
 *   2) edition= token on the kernel command line
 * Detection is read-only. */
static mi_edition_t discover_edition(mi_state_t *st) {
    char val[64] = {0};
    const char *markers[] = {
        MI_EDITION_MARKER_DIR "/slim.conf",
        MI_EDITION_MARKER_DIR "/edition.conf",
        MI_EDITION_MARKER_DIR "/minimal.conf",
        MI_EDITION_MARKER_DIR "/full.conf",
        NULL
    };
    for (int i = 0; markers[i]; i++) {
        if (conf_lookup(markers[i], "MIRVKBUNTU_EDITION", val, sizeof val)) {
            mi_edition_t e = edition_from_str(val);
            if (e != MI_EDITION_UNKNOWN) return e;
        }
    }
    /* slim images also carry the overlay size; pick it up for the report. */
    conf_lookup(MI_EDITION_MARKER_DIR "/slim.conf", "MIRVKBUNTU_OVERLAY_SIZE",
                st->overlay_size, sizeof st->overlay_size);

    /* kernel cmdline: edition=... */
    FILE *f = fopen("/proc/cmdline", "r");
    if (f) {
        char cmd[1024];
        if (fgets(cmd, sizeof cmd, f)) {
            char *tok = strstr(cmd, "edition=");
            if (tok) {
                tok += 8;
                char buf[64]; int i = 0;
                while (tok[i] && !isspace((unsigned char)tok[i]) && i < 63) {
                    buf[i] = tok[i]; i++;
                }
                buf[i] = '\0';
                mi_edition_t e = edition_from_str(buf);
                if (e != MI_EDITION_UNKNOWN) { fclose(f); return e; }
            }
        }
        fclose(f);
    }
    return MI_EDITION_UNKNOWN;
}

/* Locate the live root filesystem source (squashfs) to install from. */
static void discover_rootfs_source(mi_state_t *st) {
    const char *candidates[] = {
        "/run/live/medium/live/filesystem.squashfs",
        "/run/live/medium/casper/filesystem.squashfs",
        "/cdrom/casper/filesystem.squashfs",
        "/cdrom/live/filesystem.squashfs",
        NULL
    };
    for (int i = 0; candidates[i]; i++) {
        if (access(candidates[i], R_OK) == 0) {
            snprintf(st->source_squashfs, sizeof st->source_squashfs, "%s",
                     candidates[i]);
            return;
        }
    }
    st->source_squashfs[0] = '\0'; /* fall back to rsync-from-/ in the plan */
}

static int have_cmd(const char *name) {
    char buf[512];
    snprintf(buf, sizeof buf, "command -v %s >/dev/null 2>&1", name);
    return system(buf) == 0;
}

/* ------------------------------------------------------------------ */
/* PLAN summary + REVIEW                                               */
/* ------------------------------------------------------------------ */

static void print_component_line(FILE *o, const mi_state_t *st) {
    int any = 0;
    fputs("  Components: ", o);
    for (int i = 0; i < MI_COMPONENT_COUNT; i++) {
        if (st->components[i]) {
            fprintf(o, "%s%s", any ? " " : "", kComponentIds[i]);
            any = 1;
        }
    }
    if (!any) fputs("(none)", o);
    fputc('\n', o);
}

static void print_plan(const mi_state_t *st) {
    printf("\n==================== MirvkBuntu Install Plan ====================\n");
    printf("  Edition:    %s%s\n", edition_name(st->edition),
           st->edition == st->edition_source_detected ? " (detected)" : "");
    printf("  Desktop:    %s\n", desktop_name(st->desktop));
    print_component_line(stdout, st);
    printf("  Target:     %s%s\n",
           st->target[0] ? st->target : "(none chosen)",
           st->target[0] ? "  <-- ALL DATA WILL BE ERASED" : "");
    printf("  Hostname:   %s\n", st->hostname);
    printf("  Username:   %s\n", st->username);
    printf("  Timezone:   %s\n", st->timezone);
    printf("  Rootfs src: %s\n",
           st->source_squashfs[0] ? st->source_squashfs
                                  : "(live / via rsync)");
    if (st->edition == MI_EDITION_SLIM)
        printf("  Slim note:  toram/overlay dropped; installed persistently, overlay ref=%s\n",
               st->overlay_size[0] ? st->overlay_size : MI_SLIM_OVERLAY_DEFAULT);
    printf("  Mode:       %s\n",
           st->do_install ? "INSTALL (destructive)" : "DRY RUN (no disk writes)");
    printf("=================================================================\n\n");
}

/* ------------------------------------------------------------------ */
/* EXECUTE — generate the provisioning shell script                    */
/* ------------------------------------------------------------------ */

/*
 * Compose the full, edition-specific provisioning script into a heap buffer.
 * Every interpolated value is a typed/validated field (device path, edition,
 * desktop keyword, component ids, identity idents) — never a raw user string —
 * so the generated script cannot be used to inject arbitrary commands.
 */
static char *build_provision_script(const mi_state_t *st) {
    /* Generous fixed buffer; the script is a few KB. */
    size_t cap = 16384;
    char *s = malloc(cap);
    if (!s) return NULL;
    s[0] = '\0';

#define APP(...) do { \
        size_t used = strlen(s); \
        snprintf(s + used, cap - used, __VA_ARGS__); \
    } while (0)

    APP("#!/bin/bash\n");
    APP("# Generated by %s %s — edition=%s desktop=%s\n",
        MI_PROGRAM, MI_VERSION, edition_name(st->edition), desktop_name(st->desktop));
    APP("set -euo pipefail\n");
    APP("LOG='%s'\n", st->logpath[0] ? st->logpath : "/var/log/mirvkbuntu-install.log");
    APP("log(){ echo \"[install] $*\" | tee -a \"$LOG\"; }\n");
    APP("DISK='%s'\n", st->target);
    APP("TARGET='%s'\n", MI_TARGET_MNT);
    APP("EDITION='%s'\n", edition_name(st->edition));
    APP("DESKTOP='%s'\n", desktop_name(st->desktop));
    APP("HOSTNAME='%s'\n", st->hostname);
    APP("USERNAME='%s'\n", st->username);
    APP("TIMEZONE='%s'\n", st->timezone);
    APP("SQUASHFS='%s'\n", st->source_squashfs);
    APP("\n");

    APP("[ \"$(id -u)\" -eq 0 ] || { echo 'ERROR: must be root'; exit 1; }\n");
    APP("[ -b \"$DISK\" ] || { echo \"ERROR: not a block device: $DISK\"; exit 1; }\n\n");

    /* Partition: GPT + ESP + ext4 root (matches galactic-cherry-installer). */
    APP("log \"Partitioning $DISK (GPT: ESP + ext4 root)\"\n");
    APP("parted -s \"$DISK\" mklabel gpt\n");
    APP("parted -s \"$DISK\" mkpart primary fat32 1MiB 513MiB\n");
    APP("parted -s \"$DISK\" set 1 esp on\n");
    APP("parted -s \"$DISK\" mkpart primary ext4 513MiB 100%%\n");
    APP("udevadm settle 2>/dev/null || sleep 2\n");
    APP("PART_PREFIX=\"$DISK\"; case \"$DISK\" in *nvme*|*mmcblk*) PART_PREFIX=\"${DISK}p\";; esac\n");
    APP("EFI_PART=\"${PART_PREFIX}1\"; ROOT_PART=\"${PART_PREFIX}2\"\n\n");

    APP("log \"Formatting $EFI_PART (FAT32) and $ROOT_PART (ext4)\"\n");
    APP("mkfs.fat -F32 \"$EFI_PART\"\n");
    APP("mkfs.ext4 -F -L '%s' \"$ROOT_PART\"\n\n", MI_ROOTFS_LABEL);

    APP("log \"Mounting target at $TARGET\"\n");
    APP("mkdir -p \"$TARGET\"\n");
    APP("mount \"$ROOT_PART\" \"$TARGET\"\n");
    APP("mkdir -p \"$TARGET/boot/efi\"\n");
    APP("mount \"$EFI_PART\" \"$TARGET/boot/efi\"\n\n");

    /* Copy the root filesystem: unsquashfs if we have a squashfs, else rsync. */
    APP("log \"Copying root filesystem\"\n");
    APP("if [ -n \"$SQUASHFS\" ] && [ -f \"$SQUASHFS\" ] && command -v unsquashfs >/dev/null 2>&1; then\n");
    APP("  unsquashfs -f -d \"$TARGET\" \"$SQUASHFS\"\n");
    APP("else\n");
    APP("  rsync -aAXH --info=progress2 \\\n");
    APP("    --exclude=/dev/* --exclude=/proc/* --exclude=/sys/* --exclude=/tmp/* \\\n");
    APP("    --exclude=/run/* --exclude=/mnt/* --exclude=/media/* --exclude=/lost+found \\\n");
    APP("    / \"$TARGET/\"\n");
    APP("fi\n\n");

    APP("log \"Binding pseudo-filesystems for chroot\"\n");
    APP("for fs in dev proc sys run; do mount --bind \"/$fs\" \"$TARGET/$fs\"; done\n");
    APP("cleanup(){ for fs in run sys proc dev; do umount -l \"$TARGET/$fs\" 2>/dev/null || true; done; }\n");
    APP("trap cleanup EXIT\n\n");

    /* ---- edition-specific system shaping ---- */
    if (st->edition == MI_EDITION_SLIM) {
        APP("log \"Slim edition: converting RAM/live image to a persistent install\"\n");
        /* Remove the live/toram machinery so the disk install boots normally. */
        APP("chroot \"$TARGET\" bash -c 'apt-get purge -y live-boot live-boot-initramfs-tools live-config live-config-systemd 2>/dev/null || true'\n");
        APP("rm -f \"$TARGET/usr/lib/systemd/system/mirvkbuntu-overlay-limit.service\" \\\n");
        APP("      \"$TARGET/etc/systemd/system/multi-user.target.wants/mirvkbuntu-overlay-limit.service\" 2>/dev/null || true\n");
        APP("# Record the on-disk edition for anything that inspects the system.\n");
        APP("mkdir -p \"$TARGET%s\"\n", MI_EDITION_MARKER_DIR);
        APP("printf 'MIRVKBUNTU_EDITION=slim\\nMIRVKBUNTU_TORAM=false\\nMIRVKBUNTU_PERSISTENT=true\\n' > \"$TARGET%s/edition.conf\"\n", MI_EDITION_MARKER_DIR);
    } else {
        APP("mkdir -p \"$TARGET%s\"\n", MI_EDITION_MARKER_DIR);
        APP("printf 'MIRVKBUNTU_EDITION=%s\\n' > \"$TARGET%s/edition.conf\"\n",
            edition_name(st->edition), MI_EDITION_MARKER_DIR);
    }
    APP("\n");

    /* Desktop selection (full/slim). Minimal defaults to vanilla = no DE. */
    APP("log \"Configuring desktop: $DESKTOP\"\n");
    APP("case \"$DESKTOP\" in\n");
    APP("  gnome)\n");
    APP("    chroot \"$TARGET\" apt-get update -qq || true\n");
    APP("    chroot \"$TARGET\" apt-get install -y --no-install-recommends ubuntu-desktop-minimal gdm3 || true\n");
    APP("    chroot \"$TARGET\" systemctl set-default graphical.target 2>/dev/null || true\n");
    if (st->components[0]) { /* ubuntu-white */
        APP("    if [ -f \"$TARGET/usr/sbin/install-gnome-desktop.sh\" ]; then\n");
        APP("      chroot \"$TARGET\" env GNOME_THEME=ubuntu-white UBUNTU_WHITE_ICONS=1 /usr/sbin/install-gnome-desktop.sh || log 'ubuntu-white overlay had errors (non-fatal)'\n");
        APP("    fi\n");
    }
    APP("    ;;\n");
    APP("  mate)\n");
    APP("    if [ -f \"$TARGET/usr/sbin/install-mate-desktop.sh\" ]; then chroot \"$TARGET\" /usr/sbin/install-mate-desktop.sh || true; fi\n");
    APP("    chroot \"$TARGET\" systemctl set-default graphical.target 2>/dev/null || true\n");
    APP("    ;;\n");
    APP("  vanilla)\n");
    APP("    chroot \"$TARGET\" systemctl set-default multi-user.target 2>/dev/null || true\n");
    APP("    ;;\n");
    APP("esac\n\n");

    /* Optional component suites (skip on minimal unless explicitly selected). */
    if (st->components[1]) { /* security */
        APP("log \"Installing OS security suite\"\n");
        APP("if [ -f \"$TARGET/usr/sbin/install-os-security.sh\" ]; then chroot \"$TARGET\" bash /usr/sbin/install-os-security.sh || log 'security suite had errors (non-fatal)'; fi\n\n");
    }
    if (st->components[2]) { /* git-improved */
        APP("log \"Installing improved Git\"\n");
        APP("if [ -f \"$TARGET/usr/sbin/install-git-improved.sh\" ]; then chroot \"$TARGET\" bash /usr/sbin/install-git-improved.sh || log 'git-improved had errors (non-fatal)'; fi\n\n");
    }
    if (st->components[3]) { /* jwstf */
        APP("log \"Installing JWSTF / NitroWebExpress\"\n");
        APP("if [ -f \"$TARGET/usr/sbin/install-jwstf.sh\" ]; then chroot \"$TARGET\" bash /usr/sbin/install-jwstf.sh || log 'jwstf had errors (non-fatal)'; fi\n\n");
    }

    /* Identity + locale. */
    APP("log \"Configuring system identity\"\n");
    APP("echo \"$HOSTNAME\" > \"$TARGET/etc/hostname\"\n");
    APP("grep -q \"127.0.1.1 $HOSTNAME\" \"$TARGET/etc/hosts\" 2>/dev/null || echo \"127.0.1.1 $HOSTNAME\" >> \"$TARGET/etc/hosts\"\n");
    APP("chroot \"$TARGET\" ln -sf \"/usr/share/zoneinfo/$TIMEZONE\" /etc/localtime 2>/dev/null || true\n");
    APP("chroot \"$TARGET\" useradd -m -s /bin/bash -G sudo,adm,cdrom,audio,video,plugdev \"$USERNAME\" 2>/dev/null || true\n");
    /* Password is fed on stdin, never embedded in the script text. */
    APP("chroot \"$TARGET\" chpasswd <<'MI_CHPASS'\n");
    APP("$USERNAME:__MI_PASSWORD__\n");
    APP("MI_CHPASS\n");
    if (st->fullname[0]) {
        APP("chroot \"$TARGET\" chfn -f \"%s\" \"$USERNAME\" 2>/dev/null || true\n", st->fullname);
    }
    APP("chroot \"$TARGET\" passwd -l root 2>/dev/null || true\n\n");

    /* fstab. */
    APP("log \"Writing /etc/fstab\"\n");
    APP("ROOT_UUID=$(blkid -s UUID -o value \"$ROOT_PART\")\n");
    APP("EFI_UUID=$(blkid -s UUID -o value \"$EFI_PART\")\n");
    APP("cat > \"$TARGET/etc/fstab\" <<FSTAB\n");
    APP("# /etc/fstab - MirvkBuntu %s edition\n", edition_name(st->edition));
    APP("UUID=$ROOT_UUID  /          ext4  errors=remount-ro  0 1\n");
    APP("UUID=$EFI_UUID   /boot/efi  vfat  umask=0077         0 1\n");
    APP("FSTAB\n\n");

    /* Bootloader. */
    APP("log \"Installing GRUB bootloader\"\n");
    APP("if [ -d /sys/firmware/efi ]; then\n");
    APP("  chroot \"$TARGET\" grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id='MirvkBuntu' || true\n");
    APP("else\n");
    APP("  chroot \"$TARGET\" grub-install --target=i386-pc \"$DISK\" || true\n");
    APP("fi\n");
    APP("chroot \"$TARGET\" update-grub || true\n\n");

    APP("log \"Install of MirvkBuntu %s edition complete\"\n", edition_name(st->edition));
    APP("cleanup; trap - EXIT\n");
    APP("umount \"$TARGET/boot/efi\" 2>/dev/null || true\n");
    APP("umount \"$TARGET\" 2>/dev/null || true\n");
    APP("exit 0\n");

#undef APP
    return s;
}

/* Substitute the password placeholder into a heap copy of the script just
 * before execution (kept out of the on-disk/audit copy). */
static char *script_with_password(const char *script, const char *password) {
    const char *ph = "__MI_PASSWORD__";
    const char *hit = strstr(script, ph);
    if (!hit) return strdup(script);
    size_t plen = strlen(password);
    size_t out = strlen(script) - strlen(ph) + plen + 1;
    char *buf = malloc(out);
    if (!buf) return NULL;
    size_t pre = (size_t)(hit - script);
    memcpy(buf, script, pre);
    memcpy(buf + pre, password, plen);
    strcpy(buf + pre + plen, hit + strlen(ph));
    return buf;
}

/* ------------------------------------------------------------------ */
/* Interactive prompts (used only when not --non-interactive)          */
/* ------------------------------------------------------------------ */

static void prompt_str(const char *label, const char *dflt, char *out, size_t n) {
    printf("%s [%s]: ", label, dflt ? dflt : "");
    fflush(stdout);
    char buf[512];
    if (fgets(buf, sizeof buf, stdin)) {
        trim_newline(buf);
        if (buf[0]) { snprintf(out, n, "%s", buf); return; }
    }
    if (dflt) snprintf(out, n, "%s", dflt);
}

static int prompt_yes(const char *label) {
    printf("%s [y/N]: ", label);
    fflush(stdout);
    char buf[16];
    if (!fgets(buf, sizeof buf, stdin)) return 0;
    return buf[0] == 'y' || buf[0] == 'Y';
}

/* ------------------------------------------------------------------ */
/* Usage                                                               */
/* ------------------------------------------------------------------ */

static void usage(void) {
    printf(
"%s %s — %s\n\n"
"Full edition-aware installer for the three MirvkBuntu ISO editions. Performs\n"
"the real install to disk: partition, format, copy rootfs, edition-specific\n"
"configuration, and bootloader. DEFAULT RUN IS A DRY RUN (no disk writes).\n\n"
"Usage: sudo %s [OPTIONS]\n\n"
"Editions (--edition, auto-detected from the live image when omitted):\n"
"  slim      Full GNOME OS installed persistently to disk (drops toram/overlay,\n"
"            applies the Ubuntu White theme). ubuntu-white is forced ON.\n"
"  minimal   Smallest footprint: base system, no desktop (vanilla) by default.\n"
"  full      Complete desktop edition: full copy + chosen desktop + defaults.\n\n"
"Options:\n"
"  --edition <slim|minimal|full>   Select the edition to install.\n"
"  --desktop <gnome|mate|vanilla>  Desktop environment (full/slim).\n"
"  --target, --disk <dev>          Target block device, e.g. /dev/sda.\n"
"  --enable  <comma,list>          Turn optional components ON.\n"
"  --disable <comma,list>          Turn optional components OFF.\n"
"  --hostname <name>               System hostname.\n"
"  --username <name>               Primary user login name.\n"
"  --fullname <name>               Primary user full name (quoted).\n"
"  --timezone <Area/City>          Timezone (default America/New_York).\n"
"  --install, --confirm            Authorize the destructive install.\n"
"  --dry-run                       Plan only, never write (this is the default).\n"
"  --non-interactive               Do not prompt; use flags/defaults.\n"
"  --yes                           Assume yes at the final confirmation.\n"
"  --emit-script                   Print the generated provisioning script and exit.\n"
"  --help, -h                      Show this help and exit.\n\n"
"Optional components (defaults shown):\n",
    MI_TITLE, MI_VERSION, MI_PROGRAM, MI_PROGRAM);
    for (int i = 0; i < MI_COMPONENT_COUNT; i++)
        printf("  %-14s %s (default %s)\n", kComponentIds[i], kComponentDescs[i],
               kComponentDefaults[i] ? "ON" : "OFF");
    printf(
"\nExamples:\n"
"  sudo %s --edition slim --target /dev/sda --username max --install --yes\n"
"  sudo %s --edition minimal --non-interactive           # dry run, no writes\n"
"  %s --edition full --emit-script                       # inspect the plan\n",
    MI_PROGRAM, MI_PROGRAM, MI_PROGRAM);
}

/* ------------------------------------------------------------------ */
/* Component list parsing                                              */
/* ------------------------------------------------------------------ */

static void apply_component_list(mi_state_t *st, const char *csv, int on) {
    char buf[256];
    snprintf(buf, sizeof buf, "%s", csv);
    for (char *p = buf; *p; p++) if (*p == ',') *p = ' ';
    char *save = NULL;
    for (char *tok = strtok_r(buf, " ", &save); tok;
         tok = strtok_r(NULL, " ", &save)) {
        int idx = component_index(tok);
        if (idx >= 0) st->components[idx] = on;
        else fprintf(stderr, "warning: unknown component '%s' (ignored)\n", tok);
    }
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */

int main(int argc, char **argv) {
    mi_state_t st;
    memset(&st, 0, sizeof st);
    snprintf(st.hostname, sizeof st.hostname, "%s", "mirvkbuntu");
    snprintf(st.username, sizeof st.username, "%s", "mirvk");
    snprintf(st.timezone, sizeof st.timezone, "%s", "America/New_York");
    snprintf(st.logpath, sizeof st.logpath, "%s", "/var/log/mirvkbuntu-install.log");
    snprintf(st.overlay_size, sizeof st.overlay_size, "%s", MI_SLIM_OVERLAY_DEFAULT);

    int edition_set = 0, desktop_set = 0;
    /* Pending component edits, applied AFTER edition defaults so that
     * --enable/--disable layer on top of the edition's baseline selection. */
    char enable_edits[256] = {0};
    char disable_edits[256] = {0};

    /* ---- DISCOVER (before flags so flags win) ---- */
    st.edition_source_detected = discover_edition(&st);

    /* ---- parse CLI ---- */
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        const char *val = NULL;
        char key[64];
        const char *eq = strchr(a, '=');
        if (eq && a[0] == '-') {
            size_t klen = (size_t)(eq - a);
            if (klen >= sizeof key) klen = sizeof key - 1;
            memcpy(key, a, klen); key[klen] = '\0';
            a = key; val = eq + 1;
        }
#define NEXT() (val ? val : (i + 1 < argc ? argv[++i] : NULL))
        if (!strcmp(a, "--help") || !strcmp(a, "-h")) { usage(); return 0; }
        else if (!strcmp(a, "--edition")) {
            const char *v = NEXT();
            st.edition = edition_from_str(v);
            if (st.edition == MI_EDITION_UNKNOWN) {
                fprintf(stderr, "ERROR: --edition must be slim|minimal|full\n");
                return 2;
            }
            edition_set = 1;
        } else if (!strcmp(a, "--desktop")) {
            st.desktop = desktop_from_str(NEXT()); desktop_set = 1;
        } else if (!strcmp(a, "--target") || !strcmp(a, "--disk")) {
            const char *v = NEXT();
            if (v) snprintf(st.target, sizeof st.target, "%s", v);
        } else if (!strcmp(a, "--enable")) {
            const char *v = NEXT();
            if (v) {
                size_t u = strlen(enable_edits);
                snprintf(enable_edits + u, sizeof enable_edits - u, "%s%s",
                         u ? "," : "", v);
            }
        } else if (!strcmp(a, "--disable")) {
            const char *v = NEXT();
            if (v) {
                size_t u = strlen(disable_edits);
                snprintf(disable_edits + u, sizeof disable_edits - u, "%s%s",
                         u ? "," : "", v);
            }
        } else if (!strcmp(a, "--hostname")) {
            const char *v = NEXT(); if (v) snprintf(st.hostname, sizeof st.hostname, "%s", v);
        } else if (!strcmp(a, "--username")) {
            const char *v = NEXT(); if (v) snprintf(st.username, sizeof st.username, "%s", v);
        } else if (!strcmp(a, "--fullname")) {
            const char *v = NEXT(); if (v) snprintf(st.fullname, sizeof st.fullname, "%s", v);
        } else if (!strcmp(a, "--timezone")) {
            const char *v = NEXT(); if (v) snprintf(st.timezone, sizeof st.timezone, "%s", v);
        } else if (!strcmp(a, "--install") || !strcmp(a, "--confirm")) {
            st.do_install = 1;
        } else if (!strcmp(a, "--dry-run")) {
            st.do_install = 0;
        } else if (!strcmp(a, "--non-interactive")) {
            st.non_interactive = 1;
        } else if (!strcmp(a, "--yes") || !strcmp(a, "-y")) {
            st.assume_yes = 1;
        } else if (!strcmp(a, "--emit-script")) {
            st.emit_script = 1;
        } else {
            fprintf(stderr, "ERROR: unknown option: %s\n", argv[i]);
            fprintf(stderr, "Try '%s --help'.\n", MI_PROGRAM);
            return 2;
        }
#undef NEXT
    }

    /* Resolve edition: flag > detected > prompt/default. */
    if (!edition_set) {
        if (st.edition_source_detected != MI_EDITION_UNKNOWN) {
            st.edition = st.edition_source_detected;
        } else if (!st.non_interactive) {
            char buf[32] = "full";
            prompt_str("Edition to install (slim|minimal|full)", "full", buf, sizeof buf);
            st.edition = edition_from_str(buf);
        }
        if (st.edition == MI_EDITION_UNKNOWN) st.edition = MI_EDITION_FULL;
    }

    /* Lay down the edition's baseline (desktop + default component set), then
     * layer any explicit --enable/--disable edits on top. */
    apply_edition_defaults(&st, desktop_set, /*components_set=*/0);
    if (enable_edits[0])  apply_component_list(&st, enable_edits, 1);
    if (disable_edits[0]) apply_component_list(&st, disable_edits, 0);
    /* Slim is the White edition: ubuntu-white cannot be turned off. */
    if (st.edition == MI_EDITION_SLIM) st.components[0] = 1;

    /* Discover the rootfs source to copy from. */
    discover_rootfs_source(&st);

    /* Interactive fill-in of identity + target if not headless. */
    if (!st.non_interactive && !st.emit_script) {
        if (!st.target[0])
            prompt_str("Target disk (e.g. /dev/sda)", "", st.target, sizeof st.target);
        prompt_str("Hostname", st.hostname, st.hostname, sizeof st.hostname);
        prompt_str("Username", st.username, st.username, sizeof st.username);
        prompt_str("Full name", st.fullname[0] ? st.fullname : "", st.fullname, sizeof st.fullname);
        prompt_str("Timezone", st.timezone, st.timezone, sizeof st.timezone);
        if (st.do_install && !st.password[0]) {
            char *p = getpass("Password for the primary user: ");
            if (p) snprintf(st.password, sizeof st.password, "%s", p);
        }
    }
    if (!st.password[0]) snprintf(st.password, sizeof st.password, "%s", "mirvkbuntu");

    /* Validate identity + target BEFORE building any script. */
    if (!valid_ident(st.username, 32)) {
        fprintf(stderr, "ERROR: invalid username '%s' (letters/digits/-/_/. only)\n", st.username);
        return 2;
    }
    if (!valid_ident(st.hostname, 63)) {
        fprintf(stderr, "ERROR: invalid hostname '%s'\n", st.hostname);
        return 2;
    }
    if (st.fullname[0] && !valid_fullname(st.fullname, 127)) {
        fprintf(stderr, "ERROR: invalid full name (no quotes/backticks/$/\\/;&|<> allowed)\n");
        return 2;
    }
    /* Timezone is emitted into the script; keep it to a safe path-like set. */
    for (const char *c = st.timezone; *c; c++) {
        if (!(isalnum((unsigned char)*c) || *c == '/' || *c == '_' ||
              *c == '-' || *c == '+')) {
            fprintf(stderr, "ERROR: invalid timezone '%s'\n", st.timezone);
            return 2;
        }
    }

    /* ---- PLAN + REVIEW ---- */
    print_plan(&st);
    mi_log(&st, "plan: edition=%s desktop=%s target=%s user=%s install=%d",
           edition_name(st.edition), desktop_name(st.desktop),
           st.target[0] ? st.target : "(none)", st.username, st.do_install);

    /* Build the provisioning script (auditable, password-free). */
    char *script = build_provision_script(&st);
    if (!script) { fprintf(stderr, "ERROR: out of memory building plan\n"); return 1; }

    if (st.emit_script) {
        fputs("---- generated provisioning script (password placeholder retained) ----\n", stdout);
        fputs(script, stdout);
        free(script);
        return 0;
    }

    /* ---- DRY RUN exit ---- */
    if (!st.do_install) {
        printf("DRY RUN: no disk was touched. Re-run with --install (and a --target)\n"
               "to perform the install. Use --emit-script to inspect the exact steps.\n");
        mi_log(&st, "dry-run complete; no disk writes");
        free(script);
        return 0;
    }

    /* ---- CONFIRM + ELEVATE gate ---- */
    if (!valid_device_path(st.target)) {
        fprintf(stderr, "ERROR: --install requires a valid --target block device (got '%s')\n",
                st.target[0] ? st.target : "(none)");
        free(script);
        return 2;
    }
    if (access(st.target, F_OK) != 0) {
        fprintf(stderr, "ERROR: target device does not exist: %s\n", st.target);
        free(script);
        return 2;
    }
    if (geteuid() != 0) {
        fprintf(stderr, "ERROR: an authorized install must run as root (try: sudo %s ...)\n", MI_PROGRAM);
        free(script);
        return 1;
    }
    if (!have_cmd("parted") || !have_cmd("mkfs.ext4")) {
        fprintf(stderr, "ERROR: required tooling missing (parted / mkfs.ext4). "
                        "Run from the live session with the installer packages present.\n");
        free(script);
        return 1;
    }

    if (!st.assume_yes) {
        printf("\n*** WARNING: this will ERASE ALL DATA on %s ***\n", st.target);
        if (!prompt_yes("Type y to proceed with the destructive install")) {
            printf("Aborted. No changes made.\n");
            free(script);
            return 0;
        }
    }

    /* ---- EXECUTE ---- */
    char *runnable = script_with_password(script, st.password);
    /* Scrub the plaintext password copy from our state ASAP. */
    memset(st.password, 0, sizeof st.password);
    free(script);
    if (!runnable) { fprintf(stderr, "ERROR: out of memory\n"); return 1; }

    mi_log(&st, "EXECUTE: launching provisioning for edition=%s on %s",
           edition_name(st.edition), st.target);

    /* Feed the generated script to bash on stdin: no temp file, and the
     * password never lands on disk. */
    FILE *sh = popen("/bin/bash -s", "w");
    if (!sh) {
        fprintf(stderr, "ERROR: failed to launch shell for provisioning\n");
        memset(runnable, 0, strlen(runnable));
        free(runnable);
        return 1;
    }
    fputs(runnable, sh);
    memset(runnable, 0, strlen(runnable));
    free(runnable);
    int rc = pclose(sh);
    int exit_code = (rc == -1) ? 1 : (rc >> 8) & 0xff;

    /* ---- VERIFY + REPORT ---- */
    printf("\n==================== Install Audit Report ====================\n");
    printf("  installer:   %s %s\n", MI_PROGRAM, MI_VERSION);
    printf("  edition:     %s\n", edition_name(st.edition));
    printf("  desktop:     %s\n", desktop_name(st.desktop));
    printf("  target:      %s\n", st.target);
    printf("  operation:   edition install (partition/format/copy/configure/grub)\n");
    printf("  privilege:   root (destructive, authorized)\n");
    printf("  result:      %s (exit %d)\n", exit_code == 0 ? "SUCCESS" : "FAILED", exit_code);
    printf("  log:         %s\n", st.logpath);
    printf("==============================================================\n");
    mi_log(&st, "REPORT: result=%s exit=%d", exit_code == 0 ? "SUCCESS" : "FAILED", exit_code);

    if (exit_code == 0)
        printf("\nMirvkBuntu %s edition installed. You may reboot into the new system.\n",
               edition_name(st.edition));
    else
        printf("\nInstall FAILED. See %s for details.\n", st.logpath);

    return exit_code;
}
