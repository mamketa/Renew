/*
 * memory_utils.c — Memory subsystem tuning module for Velfox Profiler
 *
 * Responsibility: all kernel interactions related to memory management:
 *   - Virtual Memory (VM) parameter profiles (/proc/sys/vm/*)
 *   - Multi-Gen LRU enablement (MGLRU)
 *   - uclamp scheduling-class utilization clamping (cpuctl / cgroup v2)
 *   - CPU input-boost frequency and duration
 *   - I/O scheduler selection per block device
 *   - Block queue parameter tuning (read-ahead, request count)
 *   - Modern devfreq memory-bus / interconnect tuning
 *
 * All sysfs/procfs paths are sourced from velfox_paths.h macros.
 * All kernel writes go through write_sysfs() — never via fopen/fprintf.
 *
 * Standard: C23 / Clang
 */

#include "velfox_memory.h"
#include "velfox_devfreq.h"

/* =========================================================================
 * Virtual Memory profile
 * ====================================================================== */

void memory_tune_vm_profile(int mode) {
    const char *swappiness;
    const char *vfs_pressure;
    const char *compaction;
    const char *dirty_bg_ratio;
    const char *dirty_ratio;
    const char *dirty_expire;
    const char *dirty_writeback;

    switch (mode) {
        case 1:
            /* Esport — aggressive low-latency memory reclaim */
            swappiness      = "20";
            vfs_pressure    = "60";
            compaction      = "30";
            dirty_bg_ratio  = "5";
            dirty_ratio     = "15";
            dirty_expire    = "1500";
            dirty_writeback = "1500";
            break;
        case 3:
            /* Efficiency — conservative, power-aware reclaim */
            swappiness      = "50";
            vfs_pressure    = "90";
            compaction      = "25";
            dirty_bg_ratio  = "15";
            dirty_ratio     = "30";
            dirty_expire    = "4500";
            dirty_writeback = "4500";
            break;
        default:
            /* Balanced (mode 2) */
            swappiness      = "40";
            vfs_pressure    = "80";
            compaction      = "40";
            dirty_bg_ratio  = "10";
            dirty_ratio     = "25";
            dirty_expire    = "3000";
            dirty_writeback = "3000";
            break;
    }

    write_sysfs(PATH_VM_SWAPPINESS,               swappiness);
    write_sysfs(PATH_VM_VFS_CACHE_PRESSURE,        vfs_pressure);
    write_sysfs(PATH_VM_COMPACTION_PROACTIVENESS,  compaction);
    write_sysfs(PATH_VM_DIRTY_BG_RATIO,            dirty_bg_ratio);
    write_sysfs(PATH_VM_DIRTY_RATIO,               dirty_ratio);
    write_sysfs(PATH_VM_DIRTY_EXPIRE_CS,           dirty_expire);
    write_sysfs(PATH_VM_DIRTY_WRITEBACK_CS,        dirty_writeback);
}

/* =========================================================================
 * MGLRU enablement
 * ====================================================================== */

void memory_enable_mglru(void) {
    write_sysfs(PATH_MGLRU_ENABLED, "1");
}

/* =========================================================================
 * uclamp cgroup profiles
 * ====================================================================== */

/*
 * write_uclamp_pair() — Write uclamp.min and uclamp.max to both the
 * legacy cpuctl hierarchy and the cgroup v2 uclamp hierarchy for a single
 * scheduling group.
 */
static void write_uclamp_pair(const char *group,
                               const char *min_value,
                               const char *max_value) {
    char path[MAX_PATH_LEN];

    /* cgroup v1: /dev/cpuctl/<group>/cpu.uclamp.min */
    if (snprintf(path, sizeof(path), "%s/%s/cpu.uclamp.min",
                 PATH_CPUCTL_BASE, group) < (int)sizeof(path)) {
        write_sysfs(path, min_value);
    }
    /* cgroup v1: /dev/cpuctl/<group>/cpu.uclamp.max */
    if (snprintf(path, sizeof(path), "%s/%s/cpu.uclamp.max",
                 PATH_CPUCTL_BASE, group) < (int)sizeof(path)) {
        write_sysfs(path, max_value);
    }
    /* cgroup v2: /sys/fs/cgroup/uclamp/<group>.uclamp.min */
    if (snprintf(path, sizeof(path), "%s/%s.uclamp.min",
                 PATH_CGROUP_UCLAMP_BASE, group) < (int)sizeof(path)) {
        write_sysfs(path, min_value);
    }
    /* cgroup v2: /sys/fs/cgroup/uclamp/<group>.uclamp.max */
    if (snprintf(path, sizeof(path), "%s/%s.uclamp.max",
                 PATH_CGROUP_UCLAMP_BASE, group) < (int)sizeof(path)) {
        write_sysfs(path, max_value);
    }
}

void memory_tune_uclamp_profile(int mode) {
    switch (mode) {
        case 1:
            write_uclamp_pair("top-app",              "384", "1024");
            write_uclamp_pair("foreground",            "128", "1024");
            write_uclamp_pair("background",            "32",  "160");
            write_uclamp_pair("system-background",     "32",  "160");
            write_uclamp_pair("restricted-background", "0",   "128");
            break;
        case 3:
            write_uclamp_pair("top-app",              "96",  "768");
            write_uclamp_pair("foreground",            "64",  "640");
            write_uclamp_pair("background",            "16",  "192");
            write_uclamp_pair("system-background",     "16",  "192");
            write_uclamp_pair("restricted-background", "0",   "128");
            break;
        default: /* balanced */
            write_uclamp_pair("top-app",              "160", "1024");
            write_uclamp_pair("foreground",            "96",  "1024");
            write_uclamp_pair("background",            "32",  "320");
            write_uclamp_pair("system-background",     "32",  "320");
            write_uclamp_pair("restricted-background", "0",   "256");
            break;
    }
}

/* =========================================================================
 * CPU input-boost tuning
 * ====================================================================== */

void memory_tune_input_boost(int mode) {
    const char *freq;
    const char *ms;

    switch (mode) {
        case 1:
            freq = "0:1200000 1:1200000 2:1400000 3:1400000"
                   " 4:1600000 5:1600000 6:1800000 7:1800000";
            ms   = "120";
            break;
        case 3:
            freq = "0:300000 1:300000 2:500000 3:500000"
                   " 4:700000 5:700000 6:900000 7:900000";
            ms   = "40";
            break;
        default: /* balanced */
            freq = "0:800000 1:800000 2:1000000 3:1000000"
                   " 4:1200000 5:1200000 6:1400000 7:1400000";
            ms   = "80";
            break;
    }

    write_sysfs(PATH_CPU_BOOST_INPUT_FREQ,        freq);
    write_sysfs(PATH_CPU_BOOST_INPUT_MS,          ms);
    write_sysfs(PATH_CPU_INPUT_BOOST_FREQ,        freq);
    write_sysfs(PATH_CPU_INPUT_BOOST_MS,          ms);
    write_sysfs(PATH_KERNEL_CPU_INPUT_BOOST_FREQ, freq);
    write_sysfs(PATH_KERNEL_CPU_INPUT_BOOST_MS,   ms);
    write_sysfs(PATH_MSM_PERF_TOUCHBOOST,         mode == 1 ? "1" : "0");
}

/* =========================================================================
 * I/O scheduler selection
 * ====================================================================== */

/*
 * scheduler_cb() — sysfs_iter_cb for iterate_sysfs_nodes().
 * userdata points to a NULL-terminated array of candidate scheduler names.
 */
static int scheduler_cb(const char *entry_path,
                         const char *entry_name,
                         void       *userdata) {
    /* Skip pseudo-block devices that should not be tuned */
    if (strncmp(entry_name, "loop", 4) == 0 ||
        strncmp(entry_name, "ram",  3) == 0) {
        return 0;
    }

    const char **targets = (const char **)userdata;
    char scheduler_path[MAX_PATH_LEN];

    if (snprintf(scheduler_path, sizeof(scheduler_path),
                 "%s/%s", entry_path, LEAF_QUEUE_SCHEDULER) >= (int)sizeof(scheduler_path)) {
        return 0;
    }

    for (int i = 0; targets[i]; i++) {
        char matched[64];
        if (sysfs_value_supported(scheduler_path, targets[i],
                                  matched, sizeof(matched))) {
            return write_sysfs(scheduler_path,
                               matched[0] ? matched : targets[i]);
        }
    }
    return 0;
}

void memory_set_io_scheduler(int mode) {
    static const char *esport_targets[]     = { "deadline", "zen", NULL };
    static const char *balanced_targets[]   = { "cfq",              NULL };
    static const char *efficiency_targets[] = { "noop",             NULL };

    const char **targets;
    switch (mode) {
        case 1:  targets = esport_targets;     break;
        case 3:  targets = efficiency_targets; break;
        default: targets = balanced_targets;   break;
    }

    iterate_sysfs_nodes(PATH_SYSFS_BLOCK, NULL, scheduler_cb, targets);
}

/* =========================================================================
 * Block queue parameter tuning
 * ====================================================================== */

struct block_queue_data {
    const char *read_ahead;
    const char *nr_requests;
};

static int block_queue_cb(const char *entry_path,
                           const char *entry_name,
                           void       *userdata) {
    if (strncmp(entry_name, "loop", 4) == 0 ||
        strncmp(entry_name, "ram",  3) == 0) {
        return 0;
    }

    struct block_queue_data *data = (struct block_queue_data *)userdata;
    char path[MAX_PATH_LEN];

    if (snprintf(path, sizeof(path), "%s/%s", entry_path,
                 LEAF_QUEUE_READ_AHEAD_KB) < (int)sizeof(path)) {
        write_sysfs(path, data->read_ahead);
    }
    if (snprintf(path, sizeof(path), "%s/%s", entry_path,
                 LEAF_QUEUE_NR_REQUESTS) < (int)sizeof(path)) {
        write_sysfs(path, data->nr_requests);
    }
    return 1;
}

void memory_tune_block_queues(int mode) {
    struct block_queue_data data;

    switch (mode) {
        case 1:
            data.read_ahead  = "32";
            data.nr_requests = "32";
            break;
        case 3:
            data.read_ahead  = "64";
            data.nr_requests = "32";
            break;
        default: /* balanced */
            data.read_ahead  = "128";
            data.nr_requests = "64";
            break;
    }

    iterate_sysfs_nodes(PATH_SYSFS_BLOCK, NULL, block_queue_cb, &data);
}

/* =========================================================================
 * Modern devfreq tuning
 * ====================================================================== */

/*
 * devfreq_match() — Return 1 if the devfreq entry name matches a known
 * memory-bus or interconnect subsystem.
 */
static int devfreq_match(const char *name) {
    static const char *tokens[] = {
        "cpu-lat",   "cpu_lat",
        "cpu-bw",    "cpubw",
        "bw-hwmon",  "bw_hwmon",
        "llcc",      "l3",
        "memlat",
        "kgsl-ddr-qos",
        "bus_ddr",   "bus_llcc",
        "devfreq_mif",
        "dvfsrc",
        NULL
    };

    for (int i = 0; tokens[i]; i++) {
        if (strstr(name, tokens[i])) return 1;
    }
    return 0;
}

static int devfreq_cb(const char *entry_path,
                       const char *entry_name,
                       void       *userdata) {
    int mode = *((int *)userdata);

    if (!devfreq_match(entry_name)) return 0;

    switch (mode) {
        case 1:
            return LITE_MODE == 1
                   ? devfreq_mid_perf(entry_path)
                   : devfreq_max_perf(entry_path);
        case 2:
            return devfreq_unlock(entry_path);
        default: /* efficiency */
            return devfreq_mid_perf(entry_path);
    }
}

void memory_tune_devfreq(int mode) {
    /* Respect thermal mitigation flag — do not force frequencies */
    if (DEVICE_MITIGATION != 0) return;
    iterate_sysfs_nodes(PATH_SYSFS_DEVFREQ, NULL, devfreq_cb, &mode);
}
