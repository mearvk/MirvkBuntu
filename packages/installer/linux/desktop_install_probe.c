#define _POSIX_C_SOURCE 200809L
#include "desktop_install_probe.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int exists(const char *p) { struct stat s; return stat(p, &s) == 0; }
static int is_dir(const char *p) { struct stat s; return stat(p, &s) == 0 && S_ISDIR(s.st_mode); }

static int run_clone(const char *dst) {
    char cmd[PATH_MAX + 256];
    int written = snprintf(cmd, sizeof cmd, "git clone --depth 1 '%s' '%s'", DIP_REPOSITORY_URL, dst);
    if (written < 0 || (size_t)written >= sizeof cmd) {
        fputs("[error] clone command path is too long.\n", stderr);
        return -1;
    }
    puts("[run] cloning repository");
    return system(cmd);
}

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

static int join_path(char *out, size_t n, const char *base, const char *leaf) {
    int written = snprintf(out, n, "%s/%s", base, leaf);
    return written >= 0 && (size_t)written < n ? 0 : -1;
}

int main(int argc, char **argv) {
    char repo[PATH_MAX], manifest[PATH_MAX], script[PATH_MAX];
    const char *home = getenv("HOME");
    int install = argc > 1 && strcmp(argv[1], "--install") == 0;

    puts("Ubuntu Determinant — Linux Desktop Install Probe");
    puts("Step 1: locate Git clone → locate install set → optionally run installer.");

    if (locate_repo(repo, sizeof repo) != 0) {
        if (!home) { fputs("[error] HOME is unavailable.\n", stderr); return 2; }
        if (snprintf(repo, sizeof repo, "%s/%s", home, DIP_DEFAULT_CLONE_SUBPATH) >= (int)sizeof repo) {
            fputs("[error] clone destination path is too long.\n", stderr); return 2;
        }
        printf("[info] clone not found: %s\n", repo);
        if (run_clone(repo) != 0) { fputs("[error] clone failed; no installation performed.\n", stderr); return 3; }
        {
            char resolved[PATH_MAX];
            if (!realpath(repo, resolved) || strlen(resolved) >= sizeof repo) {
                fputs("[error] cloned path cannot be resolved.\n", stderr); return 3;
            }
            strcpy(repo, resolved);
        }
    }

    printf("[ok] Git clone: %s\n", repo);
    if (join_path(manifest, sizeof manifest, repo, DIP_INSTALL_MANIFEST) != 0 ||
        join_path(script, sizeof script, repo, DIP_NATIVE_INSTALLER) != 0) {
        fputs("[error] install-set path is too long.\n", stderr); return 4;
    }

    if (!exists(manifest) && !exists(script)) {
        fputs("[error] no recognized install set found.\n", stderr); return 4;
    }
    printf("[ok] Install manifest: %s%s\n", manifest, exists(manifest) ? "" : " (not present; script found)");
    printf("[ok] Desktop preview: %s/%s\n", repo, DIP_PREVIEW_DOCUMENT);

    if (!install) {
        puts("[dry-run] discovery complete; nothing installed.");
        puts("[dry-run] use --install to execute the repository's native Linux installer.");
        return 0;
    }

    if (!exists(script)) { fputs("[error] installer/install-native.sh is absent.\n", stderr); return 5; }
    printf("[install] %s\n", script);
    execl("/bin/sh", "sh", script, (char *)NULL);
    perror("execl");
    return 6;
}
