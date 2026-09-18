/*
 * drm - Ubuntu White metadata removal planner (prototype).
 *
 * IMPORTANT: this prototype does not delete files, file contents, or
 * filesystem metadata. It only validates and reports a requested removal
 * scope. A privileged destructive implementation requires an explicit,
 * separately reviewed authorization and recovery design.
 */
#include <stdio.h>
#include <string.h>

static void usage(const char *p) {
    fprintf(stderr,
        "usage: %s --class ubuntu-white [--sense 1|2|3|all] [--dry-run]\n",
        p);
}

int main(int argc, char **argv) {
    int authorized_scope = 0;
    const char *sense = "all";
    int dry_run = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--class") == 0 && i + 1 < argc) {
            if (strcmp(argv[++i], "ubuntu-white") != 0) {
                fprintf(stderr, "drm: unsupported filesystem class\n");
                return 2;
            }
            authorized_scope = 1;
        } else if (strcmp(argv[i], "--sense") == 0 && i + 1 < argc) {
            sense = argv[++i];
            if (strcmp(sense, "1") && strcmp(sense, "2") &&
                strcmp(sense, "3") && strcmp(sense, "all")) {
                fprintf(stderr, "drm: Sense must be 1, 2, 3, or all\n");
                return 2;
            }
        } else if (strcmp(argv[i], "--dry-run") == 0) {
            dry_run = 1;
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    if (!authorized_scope) {
        usage(argv[0]);
        return 1;
    }

    if (!dry_run) {
        fprintf(stderr,
            "drm: destructive removal is not enabled in this prototype.\n"
            "drm: use --dry-run to inspect the requested removal scope.\n");
        return 3;
    }

    printf("drm: DRY RUN only\n");
    printf("drm: target class = ubuntu-white\n");
    printf("drm: target Sense scope = %s\n", sense);
    printf("drm: no files or metadata were changed\n");
    return 0;
}
