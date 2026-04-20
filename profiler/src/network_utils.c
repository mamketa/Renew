/*
 * network_utils.c — Network subsystem optimization module for Velfox Profiler
 *
 * Responsibility: all kernel interactions related to network performance:
 *   - TCP Congestion Control algorithm selection (priority-order probe)
 *   - TCP/IP baseline parameter hardening (SACK, ECN, window scaling, TFO)
 *   - TCP per-profile tuning (slow-start-after-idle)
 *   - RPS (Receive Packet Steering) CPU affinity masks per rx queue
 *   - XPS (Transmit Packet Steering) CPU affinity masks per tx queue
 *   - Workqueue power-efficiency flag
 *
 * All sysfs/procfs paths are sourced from velfox_paths.h macros.
 * All kernel writes go through write_sysfs() — never via fopen/fprintf.
 *
 * Standard: C23 / Clang
 */

#include "velfox_network.h"

/* =========================================================================
 * TCP baseline tuning
 * ====================================================================== */

void network_tune_tcp_baseline(void) {
    /*
     * Probe for the best available TCP congestion control algorithm.
     * The list is ordered from most desirable (bbr3) to acceptable fallback.
     * We read tcp_available_congestion_control once and scan the result.
     */
    char available[MAX_LINE_LEN];
    read_string_from_file(available, sizeof(available),
                          PATH_NET_TCP_AVAILABLE_CC);

    static const char *algorithms[] = {
        "bbr3", "bbr2", "bbrplus", "bbr", "westwood", "cubic", NULL
    };

    for (int i = 0; algorithms[i]; i++) {
        if (string_has_token(available, algorithms[i])) {
            write_sysfs(PATH_NET_TCP_CONGESTION_CTRL, algorithms[i]);
            break;
        }
    }

    /* Reliable delivery enhancements */
    write_sysfs(PATH_NET_TCP_SACK,           "1");
    write_sysfs(PATH_NET_TCP_ECN,            "1");
    write_sysfs(PATH_NET_TCP_WINDOW_SCALING, "1");
    write_sysfs(PATH_NET_TCP_MODERATE_RCVBUF,"1");

    /* TCP Fast Open: enable for both client (1) and server (2) = mode 3 */
    write_sysfs(PATH_NET_TCP_FASTOPEN,       "3");
}

/* =========================================================================
 * TCP per-profile tuning
 * ====================================================================== */

void network_tune_tcp_profile(int mode) {
    /*
     * In esport mode, disable slow-start restart after an idle period so
     * that sustained low-latency flows (e.g. online games) do not suffer
     * a congestion-window collapse after brief inactivity.
     */
    write_sysfs(PATH_NET_TCP_SLOW_START_AFTER_IDLE, mode == 1 ? "0" : "1");
}

/* =========================================================================
 * RPS / XPS CPU affinity mask tuning
 * ====================================================================== */

/*
 * rps_xps_cb() — sysfs_iter_cb invoked once per network interface.
 * userdata points to a pre-formatted mask_hex string.
 *
 * For each rx-* queue, writes rps_cpus.
 * For each tx-* queue, writes xps_cpus.
 */
static int rps_xps_cb(const char *entry_path,
                       const char *entry_name,
                       void       *userdata) {
    (void)entry_name;
    const char *mask_hex = (const char *)userdata;

    char queue_dir[MAX_PATH_LEN];
    if (snprintf(queue_dir, sizeof(queue_dir), "%s/queues",
                 entry_path) >= (int)sizeof(queue_dir)) {
        return 0;
    }

    DIR *qdir = opendir(queue_dir);
    if (!qdir) return 0;

    struct dirent *queue;
    while ((queue = readdir(qdir)) != NULL) {
        if (!queue->d_name[0] || queue->d_name[0] == '.') continue;

        char path[MAX_PATH_LEN];
        const char *leaf = NULL;

        if (strncmp(queue->d_name, "rx-", 3) == 0) {
            leaf = LEAF_RPS_CPUS;
        } else if (strncmp(queue->d_name, "tx-", 3) == 0) {
            leaf = LEAF_XPS_CPUS;
        } else {
            continue;
        }

        if (snprintf(path, sizeof(path), "%s/%s/%s",
                     queue_dir, queue->d_name, leaf) < (int)sizeof(path)) {
            write_sysfs(path, mask_hex);
        }
    }

    closedir(qdir);
    return 1;
}

void network_tune_rps_xps(int mode) {
    long num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cpus <= 0) return;

    /* Determine how many CPUs to include in the steering mask */
    long use;
    switch (mode) {
        case 1:  use = num_cpus;               break; /* 100% — esport */
        case 2:  use = (num_cpus * 3) / 4;    break; /* 75%  — balanced */
        default: use = num_cpus / 2;           break; /* 50%  — efficiency */
    }
    if (use < 1) use = 1;

    /* Cap to the number of bits in unsigned long */
    long max_bits = (long)(sizeof(unsigned long) * 8);
    if (use > max_bits) use = max_bits;

    unsigned long mask = 0;
    for (long i = 0; i < use; i++) mask |= (1UL << i);

    char mask_hex[32];
    snprintf(mask_hex, sizeof(mask_hex), "%lx", mask);

    iterate_sysfs_nodes(PATH_SYSFS_NET, NULL, rps_xps_cb, mask_hex);
}

/* =========================================================================
 * Workqueue power-efficiency flag
 * ====================================================================== */

void network_tune_workqueue(int mode) {
    /*
     * Disable power-efficient workqueues in esport and balanced modes to
     * avoid latency introduced by CPU migration of deferred work items.
     * Enable in efficiency mode to allow the kernel to defer work to
     * already-active CPUs and reduce wake-ups.
     */
    write_sysfs(PATH_WQ_POWER_EFFICIENT, mode == 3 ? "Y" : "N");
}
