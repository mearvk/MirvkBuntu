/* Ubuntu White managed three-copy filesystem prototype. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UW_COPY_COUNT 3
#define UW_RATING_COUNT 18

struct uw_copy {
    unsigned ordinal;
    const char *path;
    unsigned long long size;
    int healthy;
};

static int valid_copy(unsigned ordinal) { return ordinal >= 1 && ordinal <= UW_COPY_COUNT; }

int main(int argc, char **argv) {
    if (argc == 1 || (argc == 2 && strcmp(argv[1], "--schema") == 0)) {
        puts("Ubuntu White physical copy model v1");
        puts("Each managed logical file may have copies 1, 2, and 3.");
        puts("Each copy has its own 18 generic ratings and health record.");
        return 0;
    }
    if (argc == 3 && strcmp(argv[1], "--copy") == 0) {
        char *end = NULL;
        unsigned long n = strtoul(argv[2], &end, 10);
        if (!end || *end || !valid_copy((unsigned)n)) {
            fprintf(stderr, "invalid copy ordinal; expected 1..3\n");
            return 2;
        }
        printf("valid Ubuntu White copy: %lu\n", n);
        return 0;
    }
    fprintf(stderr, "usage: %s [--schema | --copy 1|2|3]\n", argv[0]);
    return 1;
}
