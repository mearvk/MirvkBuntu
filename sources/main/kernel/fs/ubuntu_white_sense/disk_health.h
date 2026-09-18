#ifndef UBUNTU_WHITE_DISK_HEALTH_H
#define UBUNTU_WHITE_DISK_HEALTH_H

#include <stdint.h>

enum uw_disk_health_state {
    UW_DISK_HEALTH_UNKNOWN = 0,
    UW_DISK_HEALTH_HEALTHY,
    UW_DISK_HEALTH_WARNING,
    UW_DISK_HEALTH_UNHEALTHY
};

struct uw_disk_health {
    enum uw_disk_health_state state;
    uint64_t temperature_c;
    uint64_t power_on_hours;
    uint64_t read_errors_total;
    uint64_t read_errors_uncorrected;
    uint64_t write_errors_total;
    uint64_t write_errors_uncorrected;
    uint64_t wear_percent;
    uint64_t read_latency_max_ms;
    uint64_t write_latency_max_ms;
    uint64_t flush_latency_max_ms;
    uint64_t sampled_at_ns;
};

struct uw_file_sense_health {
    uint64_t file_identity;
    uint8_t sense_copy; /* 1..3 */
    uint8_t content_ok;
    uint8_t metadata_ok;
    uint8_t device_health_state;
    uint64_t blocks_checked;
    uint64_t read_errors_observed;
    uint64_t checksum_failures;
    uint64_t sampled_at_ns;
};

#endif
