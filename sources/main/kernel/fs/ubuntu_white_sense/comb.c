/*
 * COMB - Ubuntu White filesystem metadata collector (prototype).
 *
 * This source intentionally does not alter EXT4 on-disk structures. It provides
 * a small C collector/validator for the sidecar representation described by
 * UBUNTU-WHITE-SENSE.hsss. Kernel integration can later expose the same model
 * through a dedicated filesystem/xattr interface after review.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define COMB_SENSE_MIN 1
#define COMB_SENSE_MAX 3
#define COMB_RATING_COUNT 19

static const char *const comb_ratings[COMB_RATING_COUNT] = {
    "use", "age", "homo", "homotype", "use_2", "useage",
    "manage", "action", "lists", "calls", "actionsagainst", "same",
    "came", "come", "hold", "research", "archer-class",
    "master-manager-class", "imperial-calls"
};

static int validate_sense_count(unsigned count)
{
    return count >= COMB_SENSE_MIN && count <= COMB_SENSE_MAX;
}

static void print_schema(void)
{
    puts("COMB Ubuntu White Sense schema v1");
    puts("Sense cardinality: 1..3");
    puts("Generic ratings:");
    for (size_t i = 0; i < COMB_RATING_COUNT; ++i)
        printf("  %zu: %s\n", i + 1, comb_ratings[i]);
}

int main(int argc, char **argv)
{
    if (argc == 1 || (argc == 2 && strcmp(argv[1], "--schema") == 0)) {
        print_schema();
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "--validate-sense") == 0) {
        char *end = NULL;
        errno = 0;
        unsigned long n = strtoul(argv[2], &end, 10);
        if (errno || !end || *end || n > COMB_SENSE_MAX || !validate_sense_count((unsigned)n)) {
            fprintf(stderr, "COMB: invalid Sense count; expected 1..3\n");
            return 2;
        }
        printf("COMB: valid Sense count: %lu\n", n);
        return 0;
    }

    fprintf(stderr, "usage: %s [--schema | --validate-sense N]\n", argv[0]);
    return 1;
}
