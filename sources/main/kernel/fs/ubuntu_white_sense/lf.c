/*
 * lf - Ubuntu White companion to ls.
 * Prototype: common file data plus optional Sense metadata display.
 * This implementation reads ordinary filesystem stat data; native metadata
 * storage is deliberately deferred to the sidecar/xattr integration layer.
 */
#define _FILE_OFFSET_BITS 64
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

static const char *const ratings[18] = {
    "use", "age", "homo", "homotype", "useage", "manage", "action",
    "lists", "calls", "actionsagainst", "same", "came", "come", "hold",
    "research", "archer-class", "master-manager-class", "imperial-calls"
};

static void common(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) {
        fprintf(stderr, "lf: %s: %s\n", path, strerror(errno));
        return;
    }
    printf("%s\tsize=%lld\tmode=%o\tuid=%u\tgid=%u\n",
           path, (long long)st.st_size, (unsigned)(st.st_mode & 07777),
           (unsigned)st.st_uid, (unsigned)st.st_gid);
}

static void schema(void) {
    puts("Ubuntu White LF Sense schema v1");
    puts("Each Sense layer: 18 generic ratings + overall_health");
    for (int i = 0; i < 18; ++i) printf("%d: %s\n", i + 1, ratings[i]);
}

int main(int argc, char **argv) {
    int generic = 0, health = 0, all = 0, sense = 0, start = 1;
    if (argc == 1) { fprintf(stderr, "usage: lf [-g] [-H] [-a] [-s 1|2|3] [--schema] FILE...\n"); return 1; }
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--schema")) { schema(); return 0; }
        if (!strcmp(argv[i], "-g")) { generic = 1; continue; }
        if (!strcmp(argv[i], "-H")) { health = 1; continue; }
        if (!strcmp(argv[i], "-a")) { all = generic = health = 1; continue; }
        if (!strcmp(argv[i], "-s") && i + 1 < argc) { sense = argv[++i][0] - '0'; continue; }
        start = i; break;
    }
    if (sense < 0 || sense > 3) { fprintf(stderr, "lf: Sense must be 1, 2, or 3\n"); return 2; }
    (void)generic; (void)health; (void)all; (void)start;
    /* Metadata backend is intentionally not guessed in this prototype. */
    for (int i = start; i < argc; ++i) common(argv[i]);
    if (generic || health) puts("lf: Sense metadata display backend not yet connected; file data shown above.");
    return 0;
}
