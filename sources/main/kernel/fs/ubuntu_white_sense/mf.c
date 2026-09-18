/*
 * mf - Ubuntu White metadata modifier prototype.
 *
 * Modifies metadata only; it never edits the file payload. Native EXT4
 * integration is intentionally deferred. The current prototype validates
 * requested fields and reports the operation that a metadata backend would
 * perform.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define RATING_COUNT 18
#define SENSE_MIN 1
#define SENSE_MAX 3

static const char *const ratings[RATING_COUNT] = {
    "use", "age", "homo", "homotype", "use_2", "useage", "manage",
    "action", "lists", "calls", "actionsagainst", "same", "came", "come",
    "hold", "research", "archer-class", "master-manager-class"
};

static void usage(const char *p)
{
    printf("usage: %s [options] FILE\n", p);
    puts("  --name NAME          set metadata filename/display name");
    puts("  --date DATE          set metadata date (ISO-8601 text)");
    puts("  --author AUTHOR      set metadata author");
    puts("  --database NAME      select metadata database");
    puts("  --rating NAME VALUE  set one generic rating");
    puts("  --health VALUE       set Sense overall health");
    puts("  --sense N            select Sense layer 1..3");
    puts("  --hold TYPE          set Hold type");
    puts("  --schema             print metadata fields");
}

static int is_rating(const char *name)
{
    for (size_t i = 0; i < RATING_COUNT; ++i)
        if (strcmp(name, ratings[i]) == 0) return 1;
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--schema") == 0) {
        puts("mf Ubuntu White metadata schema v1");
        puts("address/name, date, author, database, hold, sense(1..3), health");
        puts("18 generic ratings:");
        for (size_t i = 0; i < RATING_COUNT; ++i) printf("  %s\n", ratings[i]);
        return 0;
    }

    if (argc < 2) { usage(argv[0]); return 1; }

    unsigned sense = 1;
    int changes = 0;
    const char *file = NULL;

    for (int i = 1; i < argc; ++i) {
        const char *a = argv[i];
        if (strcmp(a, "--sense") == 0) {
            if (++i >= argc) { fputs("mf: --sense requires 1..3\n", stderr); return 2; }
            char *end = NULL; errno = 0;
            unsigned long n = strtoul(argv[i], &end, 10);
            if (errno || !end || *end || n < SENSE_MIN || n > SENSE_MAX) {
                fputs("mf: invalid Sense layer; expected 1..3\n", stderr); return 2;
            }
            sense = (unsigned)n;
        } else if (strcmp(a, "--rating") == 0) {
            if (i + 2 >= argc || !is_rating(argv[i + 1])) {
                fputs("mf: --rating requires a known rating name and value\n", stderr); return 2;
            }
            i += 2; changes++;
        } else if (!strcmp(a, "--name") || !strcmp(a, "--date") ||
                   !strcmp(a, "--author") || !strcmp(a, "--database") ||
                   !strcmp(a, "--health") || !strcmp(a, "--hold")) {
            if (++i >= argc) { fprintf(stderr, "mf: %s requires a value\n", a); return 2; }
            changes++;
        } else if (a[0] != '-') {
            file = a;
        } else {
            fprintf(stderr, "mf: unknown option: %s\n", a); usage(argv[0]); return 1;
        }
    }

    if (!file) { fputs("mf: FILE is required\n", stderr); return 1; }
    if (!changes) { fputs("mf: no metadata changes requested\n", stderr); return 1; }

    printf("mf: validated metadata change for %s (Sense %u)\n", file, sense);
    puts("mf: metadata backend write is not yet enabled; file contents are unchanged.");
    return 0;
}
