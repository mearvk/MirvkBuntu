/* SPDX-License-Identifier: GPL-2.0 */
/*
 * package_installer.c — the White Edition "package-installer" ELF.
 *
 * WHAT IT IS:
 *   A small, portable C11 native binary that installs the repository's package
 *   software directly into the /user and /deck trees. It resolves what to
 *   install in one of two ways:
 *
 *     - BY DISC     (--disc <name>):     install a named bundle. The special
 *                                        disc "all" selects every component.
 *     - BY FUNCTION (--function <name>): install every component whose declared
 *                                        role/function matches the keyword
 *                                        (matched against the component id and
 *                                        its install-name).
 *
 *   Component/source/name/default data is read from
 *   installer/install-manifest.txt so this binary stays in sync with the rest
 *   of the installer.
 *
 * WHAT IT IS NOT:
 *   It is not a control plane like white-installer; it does not delegate to the
 *   Bash engine. It installs directly. It still follows the White Edition
 *   safety contract: the default action is a DRY RUN that plans and reports but
 *   writes nothing. Actual copying happens only with --install.
 *
 * Copyright (C) 2026 MEARVK LLC
 * Author: Maximilian Eric Alexander Rupplin von Keffikon
 */
#define _GNU_SOURCE
#include "package_installer.h"
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
/* Parsed manifest rows.                                              */
/* ------------------------------------------------------------------ */
typedef struct {
    char id[PKG_MAX_FIELD];        /* component id, e.g. "xmc"            */
    char source[PKG_MAX_FIELD];    /* source path, e.g. "tools/xmc"       */
    char name[PKG_MAX_FIELD];      /* install-name, e.g. "xmc"            */
    int  def;                      /* default on/off flag from manifest   */
} pkg_component;

typedef struct {
    pkg_component items[PKG_MAX_COMPONENTS];
    int count;
} pkg_manifest;

/* Which install-tree layout to target. */
typedef enum {
    PKG_EDITION_AUTO = 0,  /* detect at runtime (default)                 */
    PKG_EDITION_WHITE,     /* Ubuntu White Edition: /deck /user /system   */
    PKG_EDITION_STANDARD,  /* standard Ubuntu (FHS): /usr /sbin           */
} pkg_edition;

/* Resolved run state. */
typedef struct {
    char repo[PATH_MAX];
    char disc[PKG_MAX_FIELD];      /* --disc value (may be empty)         */
    char function[PKG_MAX_FIELD];  /* --function value (may be empty)     */
    int  do_install;               /* --install: actually copy            */
    int  list_only;                /* --list: print manifest and exit     */
    pkg_edition edition;           /* --edition / PKG_EDITION / auto      */
} run_state;

/* Resolved destination set for the active edition. */
typedef struct {
    const char *name;                   /* "white" | "standard"           */
    const char *roots[PKG_MAX_ROOTS];   /* base dirs (e.g. "/deck")       */
    const char *subdirs[PKG_MAX_ROOTS]; /* subdir under each root ("bin") */
    int count;
} pkg_destset;

/* ------------------------------------------------------------------ */
/* Small helpers.                                                     */
/* ------------------------------------------------------------------ */
static int path_exists(const char *p) { struct stat s; return stat(p, &s) == 0; }
static int is_dir(const char *p) { struct stat s; return stat(p, &s) == 0 && S_ISDIR(s.st_mode); }

static int join_path(char *out, size_t n, const char *base, const char *leaf) {
    int written = snprintf(out, n, "%s/%s", base, leaf);
    return (written >= 0 && (size_t)written < n) ? 0 : -1;
}

/* Case-insensitive substring test used for --function matching. */
static int contains_ci(const char *hay, const char *needle) {
    if (!*needle) return 1;
    for (const char *h = hay; *h; h++) {
        const char *a = h, *b = needle;
        while (*a && *b) {
            int ca = *a, cb = *b;
            if (ca >= 'A' && ca <= 'Z') ca += 32;
            if (cb >= 'A' && cb <= 'Z') cb += 32;
            if (ca != cb) break;
            a++; b++;
        }
        if (!*b) return 1;
    }
    return 0;
}

/*
 * Detect whether /etc/os-release identifies this as an Ubuntu White Edition
 * system. Case-insensitive scan for a "white" edition marker in the branding
 * fields (VARIANT=, PRETTY_NAME=, ID=, VARIANT_ID=). Read-only.
 */
static int os_release_is_white(void) {
    FILE *fp = fopen(PKG_OS_RELEASE, "r");
    if (!fp)
        return 0;
    char line[512];
    int white = 0;
    while (fgets(line, sizeof line, fp)) {
        /* Only consider the branding-relevant keys. */
        if (strncmp(line, "VARIANT", 7) != 0 &&
            strncmp(line, "PRETTY_NAME", 11) != 0 &&
            strncmp(line, "ID", 2) != 0)
            continue;
        if (contains_ci(line, "white edition") || contains_ci(line, "white-edition") ||
            contains_ci(line, "ubuntu-white") || contains_ci(line, "ubuntu white")) {
            white = 1;
            break;
        }
    }
    fclose(fp);
    return white;
}

/*
 * Resolve the effective edition, in strict precedence order so a stock system
 * is never mistaken for White Edition:
 *   1. explicit request (--edition white|standard, i.e. st->edition != AUTO);
 *   2. PKG_EDITION environment variable (white|standard);
 *   3. explicit marker file PKG_WE_MARKER present  -> white;
 *   4. /etc/os-release brands the system as White  -> white;
 *   5. both White Edition trees already exist (/deck and /user) -> white;
 *   6. otherwise -> standard (FHS: /usr, /sbin).
 */
static pkg_edition resolve_edition(const run_state *st) {
    if (st->edition == PKG_EDITION_WHITE || st->edition == PKG_EDITION_STANDARD)
        return st->edition;

    const char *env = getenv("PKG_EDITION");
    if (env) {
        if (contains_ci(env, "white"))    return PKG_EDITION_WHITE;
        if (contains_ci(env, "standard") ||
            contains_ci(env, "ubuntu") ||
            contains_ci(env, "fhs"))      return PKG_EDITION_STANDARD;
    }

    if (path_exists(PKG_WE_MARKER))
        return PKG_EDITION_WHITE;
    if (os_release_is_white())
        return PKG_EDITION_WHITE;
    if (is_dir(PKG_WE_DECK_ROOT) && is_dir(PKG_WE_USER_ROOT))
        return PKG_EDITION_WHITE;

    return PKG_EDITION_STANDARD;
}

/* Fill the destination set for the resolved edition. */
static void edition_destset(pkg_edition ed, pkg_destset *d) {
    memset(d, 0, sizeof *d);
    if (ed == PKG_EDITION_WHITE) {
        d->name = "white";
        d->roots[0] = PKG_WE_DECK_ROOT;   d->subdirs[0] = PKG_BIN_SUBDIR;
        d->roots[1] = PKG_WE_USER_ROOT;   d->subdirs[1] = PKG_BIN_SUBDIR;
        d->roots[2] = PKG_WE_SYSTEM_ROOT; d->subdirs[2] = PKG_BIN_SUBDIR;
        d->count = 3;
    } else {
        d->name = "standard";
        /* FHS: /usr/bin for ordinary tools, /sbin directly for system bins. */
        d->roots[0] = PKG_STD_USR_ROOT;   d->subdirs[0] = PKG_BIN_SUBDIR;
        d->roots[1] = PKG_STD_SBIN_ROOT;  d->subdirs[1] = "";  /* /sbin itself */
        d->count = 2;
    }
}

/*
 * Locate the Git clone exactly like desktop_install_probe / white-installer:
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

/* ------------------------------------------------------------------ */
/* Manifest parsing: component|source|install-name|default            */
/* Blank lines and '#' comments are skipped.                          */
/* ------------------------------------------------------------------ */
static int load_manifest(const char *repo, pkg_manifest *m) {
    char path[PATH_MAX];
    if (join_path(path, sizeof path, repo, PKG_INSTALL_MANIFEST) != 0) return -1;

    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "[error] cannot open manifest: %s (%s)\n", path, strerror(errno));
        return -1;
    }

    m->count = 0;
    char line[1024];
    while (fgets(line, sizeof line, fp)) {
        /* strip trailing newline */
        size_t l = strlen(line);
        while (l && (line[l - 1] == '\n' || line[l - 1] == '\r')) line[--l] = '\0';

        /* skip leading blanks */
        char *s = line;
        while (*s == ' ' || *s == '\t') s++;
        if (*s == '\0' || *s == '#') continue;

        if (m->count >= PKG_MAX_COMPONENTS) {
            fputs("[warn] manifest has more components than supported; truncating.\n", stderr);
            break;
        }

        /* split on '|' into up to 4 fields */
        char *fields[4] = { NULL, NULL, NULL, NULL };
        int nf = 0;
        char *save = NULL;
        for (char *tok = strtok_r(s, "|", &save); tok && nf < 4; tok = strtok_r(NULL, "|", &save))
            fields[nf++] = tok;

        if (nf < 3) continue; /* need at least id|source|name */

        pkg_component *c = &m->items[m->count];
        snprintf(c->id, sizeof c->id, "%s", fields[0]);
        snprintf(c->source, sizeof c->source, "%s", fields[1]);
        snprintf(c->name, sizeof c->name, "%s", fields[2]);
        c->def = (nf >= 4 && fields[3]) ? atoi(fields[3]) : 1;
        m->count++;
    }
    fclose(fp);
    return m->count > 0 ? 0 : -1;
}

/* ------------------------------------------------------------------ */
/* Selection: which components does this run install?                 */
/* Returns the number selected; marks each item via the `sel` array.  */
/* ------------------------------------------------------------------ */
static int select_components(const run_state *st, const pkg_manifest *m, int *sel) {
    int n = 0;
    for (int i = 0; i < m->count; i++) sel[i] = 0;

    if (st->disc[0]) {
        int all = (strcmp(st->disc, "all") == 0);
        for (int i = 0; i < m->count; i++) {
            /* A "disc" is a named bundle. "all" takes everything; otherwise a
             * disc name selects components whose id matches the disc name. */
            if (all || strcmp(m->items[i].id, st->disc) == 0) { sel[i] = 1; n++; }
        }
    } else if (st->function[0]) {
        for (int i = 0; i < m->count; i++) {
            if (contains_ci(m->items[i].id, st->function) ||
                contains_ci(m->items[i].name, st->function)) { sel[i] = 1; n++; }
        }
    }
    return n;
}

/* ------------------------------------------------------------------ */
/* Direct install of one artifact into a destination bin directory.   */
/* Copies repo/<source>/<name> -> <destroot>/bin/<name> (0755).       */
/* Returns 0 on success, negative on failure.                         */
/* ------------------------------------------------------------------ */
static int install_one(const char *repo, const pkg_component *c,
                       const char *destroot, const char *subdir, int do_install) {
    char src[PATH_MAX];
    char destdir[PATH_MAX];
    char dest[PATH_MAX];

    if (join_path(src, sizeof src, repo, c->source) != 0) return -1;
    /* append the install-name to the source directory */
    char srcfile[PATH_MAX];
    if (join_path(srcfile, sizeof srcfile, src, c->name) != 0) return -1;

    /* An empty subdir means install directly into destroot (e.g. /sbin). */
    if (subdir && subdir[0]) {
        if (join_path(destdir, sizeof destdir, destroot, subdir) != 0) return -1;
    } else {
        if (snprintf(destdir, sizeof destdir, "%s", destroot) >= (int)sizeof destdir)
            return -1;
    }
    if (join_path(dest, sizeof dest, destdir, c->name) != 0) return -1;

    printf("    %-12s %s -> %s\n", c->id, srcfile, dest);

    if (!do_install) return 0; /* dry-run: report only */

    if (!path_exists(srcfile)) {
        fprintf(stderr, "    [warn] source artifact not found, skipped: %s\n", srcfile);
        return -1;
    }
    if (mkdir(destroot, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "    [error] mkdir %s: %s\n", destroot, strerror(errno));
        return -1;
    }
    if (mkdir(destdir, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "    [error] mkdir %s: %s\n", destdir, strerror(errno));
        return -1;
    }

    FILE *in = fopen(srcfile, "rb");
    if (!in) { fprintf(stderr, "    [error] open %s: %s\n", srcfile, strerror(errno)); return -1; }
    FILE *out = fopen(dest, "wb");
    if (!out) { fclose(in); fprintf(stderr, "    [error] open %s: %s\n", dest, strerror(errno)); return -1; }

    char buf[65536];
    size_t r;
    int rc = 0;
    while ((r = fread(buf, 1, sizeof buf, in)) > 0) {
        if (fwrite(buf, 1, r, out) != r) { rc = -1; break; }
    }
    fclose(in);
    fclose(out);
    if (rc == 0) chmod(dest, 0755);
    else fprintf(stderr, "    [error] copy failed: %s\n", dest);
    return rc;
}

/* ------------------------------------------------------------------ */
/* Usage / help.                                                      */
/* ------------------------------------------------------------------ */
static void usage(void) {
    printf("%s %s — %s\n\n", PKG_PROGRAM, PKG_VERSION, PKG_EDITION);
    printf("Usage: %s (--disc <name> | --function <keyword>) [--install] [OPTIONS]\n\n", PKG_PROGRAM);
    puts("Installs the repository's package software DIRECTLY into edition-aware");
    puts("destination trees. Choose what to install by disc (a named bundle) or by");
    puts("function (a role/keyword matched against the manifest).");
    puts("");
    puts("Destinations are chosen by detected edition:");
    printf("  White Edition : %s/%s, %s/%s, %s/%s\n",
           PKG_WE_DECK_ROOT, PKG_BIN_SUBDIR, PKG_WE_USER_ROOT, PKG_BIN_SUBDIR,
           PKG_WE_SYSTEM_ROOT, PKG_BIN_SUBDIR);
    printf("  Standard      : %s/%s, %s\n",
           PKG_STD_USR_ROOT, PKG_BIN_SUBDIR, PKG_STD_SBIN_ROOT);
    puts("");
    puts("With no --install flag it performs a DRY RUN: it resolves and prints the");
    puts("install plan for every destination but writes nothing.");
    puts("");
    puts("Selection (choose one):");
    puts("  --disc <name>        Install the named disc/bundle. Use 'all' for every component.");
    puts("  --function <keyword> Install every component whose id/name matches the keyword.");
    puts("");
    puts("Options:");
    puts("  --edition <e>        Force destination edition: white | standard | auto");
    puts("                       (default auto; also honors the PKG_EDITION env var).");
    puts("  --install            Actually copy artifacts (default is a dry run).");
    puts("  --list               List the package manifest and exit.");
    puts("  --help, -h           Show this help and exit.");
    puts("");
    puts("Edition auto-detection precedence: --edition, then PKG_EDITION, then the");
    printf("  %s marker file, then /etc/os-release branding, then the presence of\n", PKG_WE_MARKER);
    printf("  both %s and %s; otherwise standard (FHS).\n",
           PKG_WE_DECK_ROOT, PKG_WE_USER_ROOT);
}

/* ------------------------------------------------------------------ */
/* Argument parsing. Accepts both `--flag value` and `--flag=value`.  */
/* Returns 0 ok, negative on error, positive to exit-0 (help).        */
/* ------------------------------------------------------------------ */
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
        } else if (strcmp(arg, "--disc") == 0) {
            const char *v = inlineval ? inlineval : (++i < argc ? argv[i] : NULL);
            if (!v) { fprintf(stderr, "ERROR: --disc requires a value.\n"); return -1; }
            snprintf(st->disc, sizeof st->disc, "%s", v);
        } else if (strcmp(arg, "--function") == 0) {
            const char *v = inlineval ? inlineval : (++i < argc ? argv[i] : NULL);
            if (!v) { fprintf(stderr, "ERROR: --function requires a value.\n"); return -1; }
            snprintf(st->function, sizeof st->function, "%s", v);
        } else if (strcmp(arg, "--install") == 0 || strcmp(arg, "--confirm") == 0) {
            st->do_install = 1;
        } else if (strcmp(arg, "--dry-run") == 0) {
            st->do_install = 0;
        } else if (strcmp(arg, "--list") == 0) {
            st->list_only = 1;
        } else if (strcmp(arg, "--edition") == 0) {
            const char *v = inlineval ? inlineval : (++i < argc ? argv[i] : NULL);
            if (!v) { fprintf(stderr, "ERROR: --edition requires white|standard|auto.\n"); return -1; }
            if (contains_ci(v, "white"))         st->edition = PKG_EDITION_WHITE;
            else if (contains_ci(v, "standard") ||
                     contains_ci(v, "ubuntu") ||
                     contains_ci(v, "fhs"))      st->edition = PKG_EDITION_STANDARD;
            else if (contains_ci(v, "auto"))     st->edition = PKG_EDITION_AUTO;
            else { fprintf(stderr, "ERROR: unknown --edition '%s' (use white|standard|auto).\n", v); return -1; }
        } else {
            fprintf(stderr, "ERROR: Unknown option: %s\n", argv[i]);
            fprintf(stderr, "Try '%s --help' for usage.\n", PKG_PROGRAM);
            return -1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Audit report (ARCHITECTURE.md section-7 fields, no secrets/PII).   */
/* ------------------------------------------------------------------ */
static void iso_time(char *out, size_t n) {
    time_t t = time(NULL);
    struct tm g;
    gmtime_r(&t, &g);
    strftime(out, n, "%Y-%m-%dT%H:%M:%SZ", &g);
}

int main(int argc, char **argv) {
    run_state st;
    memset(&st, 0, sizeof st);

    char start_iso[32];
    iso_time(start_iso, sizeof start_iso);

    int pr = parse_args(argc, argv, &st);
    if (pr > 0) return 0;   /* --help printed */
    if (pr < 0) return 2;   /* bad args       */

    printf("%s %s — %s\n", PKG_PROGRAM, PKG_VERSION, PKG_EDITION);

    if (locate_repo(st.repo, sizeof st.repo) != 0) {
        fputs("[error] could not locate the repository clone (no .git found).\n", stderr);
        return 3;
    }
    printf("[ok] Git clone: %s\n", st.repo);

    pkg_manifest m;
    if (load_manifest(st.repo, &m) != 0) {
        fputs("[error] could not load a usable install manifest.\n", stderr);
        return 4;
    }
    printf("[ok] Manifest: %d component(s) from %s\n", m.count, PKG_INSTALL_MANIFEST);

    if (st.list_only) {
        puts("");
        puts("== Package manifest ==");
        printf("  %-12s %-16s %-16s %s\n", "id", "source", "install-name", "default");
        for (int i = 0; i < m.count; i++)
            printf("  %-12s %-16s %-16s %s\n",
                   m.items[i].id, m.items[i].source, m.items[i].name,
                   m.items[i].def ? "on" : "off");
        return 0;
    }

    if (st.disc[0] && st.function[0]) {
        fputs("[error] choose only one of --disc or --function.\n", stderr);
        return 2;
    }
    if (!st.disc[0] && !st.function[0]) {
        fputs("[error] nothing selected; pass --disc <name> or --function <keyword>.\n", stderr);
        fprintf(stderr, "Try '%s --help' for usage.\n", PKG_PROGRAM);
        return 2;
    }

    int sel[PKG_MAX_COMPONENTS];
    int nsel = select_components(&st, &m, sel);
    if (nsel == 0) {
        fprintf(stderr, "[error] no components matched %s '%s'.\n",
                st.disc[0] ? "disc" : "function",
                st.disc[0] ? st.disc : st.function);
        return 5;
    }

    /* Resolve the edition-aware destination set. */
    pkg_edition ed = resolve_edition(&st);
    pkg_destset dest;
    edition_destset(ed, &dest);
    printf("[ok] Edition: %s%s\n", dest.name,
           st.edition == PKG_EDITION_AUTO ? " (auto-detected)" : " (forced)");

    puts("");
    printf("== Plan: install %d component(s) by %s '%s' ==\n",
           nsel, st.disc[0] ? "disc" : "function",
           st.disc[0] ? st.disc : st.function);
    printf("%s\n", st.do_install ? "[install] copying artifacts:" : "[dry-run] would install (no changes):");

    int failures = 0, actions = 0;
    for (int r = 0; r < dest.count; r++) {
        const char *root = dest.roots[r];
        const char *sub  = dest.subdirs[r];
        if (sub && sub[0])
            printf("  destination %s/%s:\n", root, sub);
        else
            printf("  destination %s:\n", root);
        for (int i = 0; i < m.count; i++) {
            if (!sel[i]) continue;
            actions++;
            if (install_one(st.repo, &m.items[i], root, sub, st.do_install) != 0 && st.do_install)
                failures++;
        }
    }

    char end_iso[32];
    iso_time(end_iso, sizeof end_iso);

    puts("");
    puts("== AUDIT REPORT ==");
    puts("----------------------------------------------------------------");
    printf("installer version      : %s %s\n", PKG_PROGRAM, PKG_VERSION);
    printf("source                 : %s (%s)\n", st.repo, DIP_REPOSITORY_URL);
    printf("selection mode         : %s\n", st.disc[0] ? "disc" : "function");
    printf("selection value        : %s\n", st.disc[0] ? st.disc : st.function);
    printf("components selected    : %d\n", nsel);
    printf("edition                : %s%s\n", dest.name,
           st.edition == PKG_EDITION_AUTO ? " (auto-detected)" : " (forced)");
    printf("destinations           : ");
    for (int r = 0; r < dest.count; r++) {
        if (dest.subdirs[r] && dest.subdirs[r][0])
            printf("%s%s/%s", r ? ", " : "", dest.roots[r], dest.subdirs[r]);
        else
            printf("%s%s", r ? ", " : "", dest.roots[r]);
    }
    printf("\n");
    printf("operation              : %s\n",
           st.do_install ? "direct install" : "dry-run (plan only)");
    printf("planned actions        : %d\n", actions);
    printf("start/end time         : %s / %s\n", start_iso, end_iso);
    if (st.do_install)
        printf("verification result    : %d action(s), %d failure(s)\n", actions, failures);
    else
        printf("verification result    : plan built ok (no changes)\n");
    puts("no secrets/PII in this report (credentials are never collected here).");
    puts("----------------------------------------------------------------");

    return failures ? 6 : 0;
}
