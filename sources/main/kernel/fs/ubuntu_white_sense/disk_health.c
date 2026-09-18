/* Ubuntu White disk/file-copy health vocabulary and collector boundary. */
#include "disk_health.h"
#include <stdio.h>

const char *uw_disk_health_name(enum uw_disk_health_state state)
{
    switch (state) {
    case UW_DISK_HEALTH_HEALTHY: return "healthy";
    case UW_DISK_HEALTH_WARNING: return "warning";
    case UW_DISK_HEALTH_UNHEALTHY: return "unhealthy";
    default: return "unknown";
    }
}

int main(void)
{
    puts("Ubuntu White Disk Health v1");
    puts("Device health is sampled from the storage stack/device interface.");
    puts("File Sense health additionally records observed read/checksum integrity.");
    puts("SMART-style counters are evidence, not an absolute prediction of failure.");
    return 0;
}
