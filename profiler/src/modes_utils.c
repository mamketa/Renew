#include "velfox_modes.h"
#include "velfox_cpufreq.h"
#include "velfox_devfreq.h"
#include "velfox_soc.h"

int SOC = 0;
int LITE_MODE = 0;
int DEVICE_MITIGATION = 0;
char DEFAULT_CPU_GOV[50] = "schedutil";
char PPM_POLICY[512] = "";

static void run_soc_mode(int mode);
static void set_io_scheduler_mode(int mode);
static void tune_block_queues(int mode);
static void tune_uclamp_profile(int mode);
static void tune_input_boost(int mode);
static void tune_modern_devfreq(int mode);
static void enable_mglru(void);

void read_configs() {
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s/soc_recognition", MODULE_CONFIG);
    SOC = read_int_from_file(path);
    snprintf(path, sizeof(path), "%s/lite_mode", MODULE_CONFIG);
    LITE_MODE = read_int_from_file(path);
    snprintf(path, sizeof(path), "%s/ppm_policies_mediatek", MODULE_CONFIG);
    read_string_from_file(PPM_POLICY, sizeof(PPM_POLICY), path);
    snprintf(path, sizeof(path), "%s/custom_default_cpu_gov", MODULE_CONFIG);
    if (file_exists(path)) {
        read_string_from_file(DEFAULT_CPU_GOV, sizeof(DEFAULT_CPU_GOV), path);
    } else {
        snprintf(path, sizeof(path), "%s/default_cpu_gov", MODULE_CONFIG);
        read_string_from_file(DEFAULT_CPU_GOV, sizeof(DEFAULT_CPU_GOV), path);
    }
    if (!DEFAULT_CPU_GOV[0]) snprintf(DEFAULT_CPU_GOV, sizeof(DEFAULT_CPU_GOV), "schedutil");
    snprintf(path, sizeof(path), "%s/device_mitigation", MODULE_CONFIG);
    DEVICE_MITIGATION = read_int_from_file(path);
}

void set_dnd(int mode) {
    if (mode == 0) system("cmd notification set_dnd off");
    else if (mode == 1) system("cmd notification set_dnd priority");
}

void set_rps_xps(int mode) {
    int cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (cores <= 0) return;
    int use = cores;
    if (mode == 2) use = (cores * 3) / 4;
    else if (mode == 3) use = cores / 2;
    if (use < 1) use = 1;
    if (use > (int)(sizeof(unsigned long) * 8)) use = sizeof(unsigned long) * 8;
    unsigned long mask = 0;
    for (int i = 0; i < use; i++) mask |= (1UL << i);
    char mask_hex[32];
    snprintf(mask_hex, sizeof(mask_hex), "%lx", mask);
    DIR *netdir = opendir("/sys/class/net");
    if (!netdir) return;
    struct dirent *iface;
    while ((iface = readdir(netdir)) != NULL) {
        if (!iface->d_name[0] || iface->d_name[0] == '.') continue;
        char queue_dir[MAX_PATH_LEN];
        if (snprintf(queue_dir, sizeof(queue_dir), "/sys/class/net/%s/queues", iface->d_name) >= (int)sizeof(queue_dir)) continue;
        DIR *qdir = opendir(queue_dir);
        if (!qdir) continue;
        struct dirent *queue;
        while ((queue = readdir(qdir)) != NULL) {
            if (!queue->d_name[0] || queue->d_name[0] == '.') continue;
            char path[MAX_PATH_LEN];
            if (strncmp(queue->d_name, "rx-", 3) == 0) {
                if (snprintf(path, sizeof(path), "%s/%s/rps_cpus", queue_dir, queue->d_name) < (int)sizeof(path)) write_sysfs(path, mask_hex);
            } else if (strncmp(queue->d_name, "tx-", 3) == 0) {
                if (snprintf(path, sizeof(path), "%s/%s/xps_cpus", queue_dir, queue->d_name) < (int)sizeof(path)) write_sysfs(path, mask_hex);
            }
        }
        closedir(qdir);
    }
    closedir(netdir);
}

static int block_common_cb(const char *entry_path, const char *entry_name, void *userdata) {
    (void)userdata;
    if (strncmp(entry_name, "loop", 4) == 0 || strncmp(entry_name, "ram", 3) == 0) return 0;
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s/queue/iostats", entry_path);
    write_sysfs(path, "0");
    snprintf(path, sizeof(path), "%s/queue/add_random", entry_path);
    write_sysfs(path, "0");
    return 1;
}

static int thermal_policy_cb(const char *entry_path, const char *entry_name, void *userdata) {
    (void)entry_name;
    (void)userdata;
    char path[MAX_PATH_LEN];
    if (!path_join(path, sizeof(path), entry_path, "policy")) return 0;
    return write_sysfs(path, "step_wise");
}

void perfcommon() {
    write_sysfs("/proc/sys/kernel/panic", "0");
    write_sysfs("/proc/sys/kernel/panic_on_oops", "0");
    write_sysfs("/proc/sys/kernel/panic_on_warn", "0");
    write_sysfs("/proc/sys/kernel/softlockup_panic", "0");
    sync();
    iterate_sysfs_nodes("/sys/block", NULL, block_common_cb, NULL);
    char tcp_available[MAX_LINE_LEN];
    read_string_from_file(tcp_available, sizeof(tcp_available), "/proc/sys/net/ipv4/tcp_available_congestion_control");
    const char *algorithms[] = {"bbr3", "bbr2", "bbrplus", "bbr", "westwood", "cubic"};
    for (size_t i = 0; i < sizeof(algorithms) / sizeof(algorithms[0]); i++) {
        if (string_has_token(tcp_available, algorithms[i])) {
            write_sysfs("/proc/sys/net/ipv4/tcp_congestion_control", algorithms[i]);
            break;
        }
    }
    write_sysfs("/proc/sys/net/ipv4/tcp_sack", "1");
    write_sysfs("/proc/sys/net/ipv4/tcp_ecn", "1");
    write_sysfs("/proc/sys/net/ipv4/tcp_window_scaling", "1");
    write_sysfs("/proc/sys/net/ipv4/tcp_moderate_rcvbuf", "1");
    write_sysfs("/proc/sys/net/ipv4/tcp_fastopen", "3");
    write_sysfs("/proc/sys/kernel/perf_cpu_time_max_percent", "3");
    write_sysfs("/proc/sys/kernel/sched_schedstats", "0");
    write_sysfs("/proc/sys/kernel/sched_autogroup_enabled", "0");
    write_sysfs("/proc/sys/kernel/sched_child_runs_first", "1");
    write_sysfs("/proc/sys/kernel/task_cpustats_enable", "0");
    write_sysfs("/sys/module/mmc_core/parameters/use_spi_crc", "0");
    write_sysfs("/sys/module/opchain/parameters/chain_on", "0");
    write_sysfs("/sys/module/cpufreq_bouncing/parameters/enable", "0");
    write_sysfs("/proc/task_info/task_sched_info/task_sched_info_enable", "0");
    write_sysfs("/proc/oplus_scheduler/sched_assist/sched_assist_enabled", "0");
    write_sysfs("/proc/sys/kernel/printk", "0 0 0 0");
    write_sysfs("/dev/cpuset/background/memory_spread_page", "0");
    write_sysfs("/dev/cpuset/system-background/memory_spread_page", "0");
    write_sysfs("/proc/sys/vm/page-cluster", "0");
    write_sysfs("/proc/sys/vm/stat_interval", "15");
    write_sysfs("/proc/sys/vm/overcommit_ratio", "80");
    write_sysfs("/proc/sys/vm/extfrag_threshold", "640");
    write_sysfs("/proc/sys/vm/watermark_scale_factor", "22");
    write_sysfs("/proc/sys/kernel/sched_lib_name", "libunity.so, libil2cpp.so, libmain.so, libUE4.so, libgodot_android.so, libgdx.so, libgdx-box2d.so, libminecraftpe.so, libLive2DCubismCore.so, libyuzu-android.so, libryujinx.so, libcitra-android.so, libhdr_pro_engine.so, libandroidx.graphics.path.so, libeffect.so");
    write_sysfs("/proc/sys/kernel/sched_lib_mask_force", "255");
    iterate_sysfs_nodes("/sys/class/thermal", "thermal_zone", thermal_policy_cb, NULL);
    enable_mglru();
}

static void maybe_set_dnd_from_config(int enabled) {
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s/dnd_gameplay", MODULE_CONFIG);
    if (read_int_from_file(path) == 1) set_dnd(enabled);
}

void esport_mode() {
    maybe_set_dnd_from_config(1);
    write_sysfs("/sys/module/workqueue/parameters/power_efficient", "N");
    set_rps_xps(1);
    write_sysfs("/proc/sys/net/ipv4/tcp_slow_start_after_idle", "0");
    write_sysfs("/proc/sys/kernel/split_lock_mitigate", "0");
    write_sysfs("/proc/sys/kernel/sched_energy_aware", "0");
    write_sysfs("/proc/sys/kernel/sched_boost", "1");
    write_sysfs("/dev/stune/top-app/schedtune.prefer_idle", "1");
    write_sysfs("/dev/stune/top-app/schedtune.boost", "1");
    write_sysfs("/proc/sys/kernel/sched_nr_migrate", "32");
    write_sysfs("/proc/sys/kernel/sched_rr_timeslice_ms", "4");
    write_sysfs("/proc/sys/kernel/sched_time_avg_ms", "80");
    write_sysfs("/proc/sys/kernel/sched_tunable_scaling", "1");
    write_sysfs("/proc/sys/kernel/sched_min_task_util_for_boost", "15");
    write_sysfs("/proc/sys/kernel/sched_min_task_util_for_colocation", "8");
    write_sysfs("/proc/sys/kernel/sched_migration_cost_ns", "50000");
    write_sysfs("/proc/sys/kernel/sched_min_granularity_ns", "800000");
    write_sysfs("/proc/sys/kernel/sched_wakeup_granularity_ns", "900000");
    write_sysfs("/sys/kernel/debug/sched_features", "NEXT_BUDDY");
    write_sysfs("/sys/kernel/debug/sched_features", "NO_TTWU_QUEUE");
    write_sysfs("/dev/cpuset/top-app/memory_spread_page", "1");
    write_sysfs("/dev/cpuset/foreground/memory_spread_page", "1");
    write_sysfs("/proc/touchpanel/game_switch_enable", "1");
    write_sysfs("/proc/touchpanel/oplus_tp_limit_enable", "0");
    write_sysfs("/proc/touchpanel/oppo_tp_limit_enable", "0");
    write_sysfs("/proc/touchpanel/oplus_tp_direction", "1");
    write_sysfs("/proc/touchpanel/oppo_tp_direction", "1");
    if (LITE_MODE == 0 && DEVICE_MITIGATION == 0) change_cpu_gov("performance");
    else change_cpu_gov(DEFAULT_CPU_GOV);
    if (file_exists("/proc/ppm")) cpufreq_ppm_max_perf();
    else cpufreq_max_perf();
    tune_uclamp_profile(1);
    tune_input_boost(1);
    tune_modern_devfreq(1);
    tune_block_queues(1);
    set_io_scheduler_mode(1);
    write_sysfs("/proc/sys/vm/swappiness", "20");
    write_sysfs("/proc/sys/vm/vfs_cache_pressure", "60");
    write_sysfs("/proc/sys/vm/compaction_proactiveness", "30");
    write_sysfs("/proc/sys/vm/dirty_background_ratio", "5");
    write_sysfs("/proc/sys/vm/dirty_ratio", "15");
    write_sysfs("/proc/sys/vm/dirty_expire_centisecs", "1500");
    write_sysfs("/proc/sys/vm/dirty_writeback_centisecs", "1500");
    run_soc_mode(1);
}

void balanced_mode() {
    maybe_set_dnd_from_config(0);
    write_sysfs("/sys/module/workqueue/parameters/power_efficient", "N");
    set_rps_xps(2);
    write_sysfs("/proc/sys/net/ipv4/tcp_slow_start_after_idle", "1");
    write_sysfs("/proc/sys/kernel/split_lock_mitigate", "1");
    write_sysfs("/proc/sys/kernel/sched_energy_aware", "1");
    write_sysfs("/proc/sys/kernel/sched_boost", "0");
    write_sysfs("/dev/stune/top-app/schedtune.prefer_idle", "0");
    write_sysfs("/dev/stune/top-app/schedtune.boost", "1");
    write_sysfs("/proc/sys/kernel/sched_rr_timeslice_ms", "6");
    write_sysfs("/proc/sys/kernel/sched_time_avg_ms", "100");
    write_sysfs("/proc/sys/kernel/sched_tunable_scaling", "1");
    write_sysfs("/proc/sys/kernel/sched_nr_migrate", "16");
    write_sysfs("/proc/sys/kernel/sched_min_task_util_for_boost", "25");
    write_sysfs("/proc/sys/kernel/sched_min_task_util_for_colocation", "15");
    write_sysfs("/proc/sys/kernel/sched_migration_cost_ns", "100000");
    write_sysfs("/proc/sys/kernel/sched_min_granularity_ns", "1200000");
    write_sysfs("/proc/sys/kernel/sched_wakeup_granularity_ns", "2000000");
    write_sysfs("/sys/kernel/debug/sched_features", "NEXT_BUDDY");
    write_sysfs("/sys/kernel/debug/sched_features", "TTWU_QUEUE");
    write_sysfs("/dev/cpuset/top-app/memory_spread_page", "1");
    write_sysfs("/dev/cpuset/foreground/memory_spread_page", "0");
    write_sysfs("/proc/touchpanel/game_switch_enable", "0");
    write_sysfs("/proc/touchpanel/oplus_tp_limit_enable", "1");
    write_sysfs("/proc/touchpanel/oppo_tp_limit_enable", "1");
    write_sysfs("/proc/touchpanel/oplus_tp_direction", "0");
    write_sysfs("/proc/touchpanel/oppo_tp_direction", "0");
    change_cpu_gov(DEFAULT_CPU_GOV);
    if (file_exists("/proc/ppm")) cpufreq_ppm_unlock();
    else cpufreq_unlock();
    tune_uclamp_profile(2);
    tune_input_boost(2);
    tune_modern_devfreq(2);
    tune_block_queues(2);
    set_io_scheduler_mode(2);
    write_sysfs("/proc/sys/vm/swappiness", "40");
    write_sysfs("/proc/sys/vm/vfs_cache_pressure", "80");
    write_sysfs("/proc/sys/vm/compaction_proactiveness", "40");
    write_sysfs("/proc/sys/vm/dirty_background_ratio", "10");
    write_sysfs("/proc/sys/vm/dirty_ratio", "25");
    write_sysfs("/proc/sys/vm/dirty_expire_centisecs", "3000");
    write_sysfs("/proc/sys/vm/dirty_writeback_centisecs", "3000");
    run_soc_mode(2);
}

void efficiency_mode() {
    maybe_set_dnd_from_config(0);
    write_sysfs("/sys/module/workqueue/parameters/power_efficient", "Y");
    set_rps_xps(3);
    write_sysfs("/proc/sys/net/ipv4/tcp_slow_start_after_idle", "1");
    write_sysfs("/proc/sys/kernel/split_lock_mitigate", "1");
    write_sysfs("/proc/sys/kernel/sched_energy_aware", "1");
    write_sysfs("/proc/sys/kernel/sched_boost", "0");
    write_sysfs("/dev/stune/top-app/schedtune.prefer_idle", "1");
    write_sysfs("/dev/stune/top-app/schedtune.boost", "0");
    write_sysfs("/proc/sys/kernel/sched_rr_timeslice_ms", "8");
    write_sysfs("/proc/sys/kernel/sched_time_avg_ms", "120");
    write_sysfs("/proc/sys/kernel/sched_tunable_scaling", "1");
    write_sysfs("/proc/sys/kernel/sched_nr_migrate", "8");
    write_sysfs("/proc/sys/kernel/sched_min_task_util_for_boost", "35");
    write_sysfs("/proc/sys/kernel/sched_min_task_util_for_colocation", "22");
    write_sysfs("/proc/sys/kernel/sched_migration_cost_ns", "180000");
    write_sysfs("/proc/sys/kernel/sched_min_granularity_ns", "1800000");
    write_sysfs("/proc/sys/kernel/sched_wakeup_granularity_ns", "2600000");
    write_sysfs("/sys/kernel/debug/sched_features", "NO_NEXT_BUDDY");
    write_sysfs("/sys/kernel/debug/sched_features", "TTWU_QUEUE");
    write_sysfs("/dev/cpuset/top-app/memory_spread_page", "0");
    write_sysfs("/dev/cpuset/foreground/memory_spread_page", "0");
    write_sysfs("/proc/touchpanel/game_switch_enable", "0");
    write_sysfs("/proc/touchpanel/oplus_tp_limit_enable", "1");
    write_sysfs("/proc/touchpanel/oppo_tp_limit_enable", "1");
    write_sysfs("/proc/touchpanel/oplus_tp_direction", "0");
    write_sysfs("/proc/touchpanel/oppo_tp_direction", "0");
    char efficiency_gov_path[MAX_PATH_LEN];
    char efficiency_gov[50];
    snprintf(efficiency_gov_path, sizeof(efficiency_gov_path), "%s/efficiency_cpu_gov", MODULE_CONFIG);
    read_string_from_file(efficiency_gov, sizeof(efficiency_gov), efficiency_gov_path);
    change_cpu_gov(efficiency_gov[0] ? efficiency_gov : DEFAULT_CPU_GOV);
    if (file_exists("/proc/ppm")) cpufreq_ppm_unlock();
    else cpufreq_unlock();
    tune_uclamp_profile(3);
    tune_input_boost(3);
    tune_modern_devfreq(3);
    tune_block_queues(3);
    set_io_scheduler_mode(3);
    write_sysfs("/proc/sys/vm/swappiness", "50");
    write_sysfs("/proc/sys/vm/vfs_cache_pressure", "90");
    write_sysfs("/proc/sys/vm/compaction_proactiveness", "25");
    write_sysfs("/proc/sys/vm/dirty_background_ratio", "15");
    write_sysfs("/proc/sys/vm/dirty_ratio", "30");
    write_sysfs("/proc/sys/vm/dirty_writeback_centisecs", "4500");
    write_sysfs("/proc/sys/vm/dirty_expire_centisecs", "4500");
    run_soc_mode(3);
}

static void run_soc_mode(int mode) {
    switch (SOC) {
        case 1: if (mode == 1) mediatek_esport(); else if (mode == 2) mediatek_balanced(); else mediatek_efficiency(); break;
        case 2: if (mode == 1) snapdragon_esport(); else if (mode == 2) snapdragon_balanced(); else snapdragon_efficiency(); break;
        case 3: if (mode == 1) exynos_esport(); else if (mode == 2) exynos_balanced(); else exynos_efficiency(); break;
        case 4: if (mode == 1) unisoc_esport(); else if (mode == 2) unisoc_balanced(); else unisoc_efficiency(); break;
        case 5: if (mode == 1) tensor_esport(); else if (mode == 2) tensor_balanced(); else tensor_efficiency(); break;
    }
}

static void enable_mglru(void) {
    write_sysfs("/sys/kernel/mm/lru_gen/enabled", "1");
}

static void write_uclamp_pair(const char *group, const char *min_value, const char *max_value) {
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "/dev/cpuctl/%s/cpu.uclamp.min", group);
    write_sysfs(path, min_value);
    snprintf(path, sizeof(path), "/dev/cpuctl/%s/cpu.uclamp.max", group);
    write_sysfs(path, max_value);
    snprintf(path, sizeof(path), "/sys/fs/cgroup/uclamp/%s.uclamp.min", group);
    write_sysfs(path, min_value);
    snprintf(path, sizeof(path), "/sys/fs/cgroup/uclamp/%s.uclamp.max", group);
    write_sysfs(path, max_value);
}

static void tune_uclamp_profile(int mode) {
    if (mode == 1) {
        write_uclamp_pair("top-app", "384", "1024");
        write_uclamp_pair("foreground", "128", "1024");
        write_uclamp_pair("background", "32", "160");
        write_uclamp_pair("system-background", "32", "160");
        write_uclamp_pair("restricted-background", "0", "128");
    } else if (mode == 2) {
        write_uclamp_pair("top-app", "160", "1024");
        write_uclamp_pair("foreground", "96", "1024");
        write_uclamp_pair("background", "32", "320");
        write_uclamp_pair("system-background", "32", "320");
        write_uclamp_pair("restricted-background", "0", "256");
    } else {
        write_uclamp_pair("top-app", "96", "768");
        write_uclamp_pair("foreground", "64", "640");
        write_uclamp_pair("background", "16", "192");
        write_uclamp_pair("system-background", "16", "192");
        write_uclamp_pair("restricted-background", "0", "128");
    }
}

static void tune_input_boost(int mode) {
    const char *freq = "0:800000 1:800000 2:1000000 3:1000000 4:1200000 5:1200000 6:1400000 7:1400000";
    const char *ms = "80";
    if (mode == 1) {
        freq = "0:1200000 1:1200000 2:1400000 3:1400000 4:1600000 5:1600000 6:1800000 7:1800000";
        ms = "120";
    } else if (mode == 3) {
        freq = "0:300000 1:300000 2:500000 3:500000 4:700000 5:700000 6:900000 7:900000";
        ms = "40";
    }
    write_sysfs("/sys/module/cpu_boost/parameters/input_boost_freq", freq);
    write_sysfs("/sys/module/cpu_boost/parameters/input_boost_ms", ms);
    write_sysfs("/sys/module/cpu_input_boost/parameters/input_boost_freq", freq);
    write_sysfs("/sys/module/cpu_input_boost/parameters/input_boost_ms", ms);
    write_sysfs("/sys/kernel/cpu_input_boost/input_boost_freq", freq);
    write_sysfs("/sys/kernel/cpu_input_boost/input_boost_ms", ms);
    write_sysfs("/sys/module/msm_performance/parameters/touchboost", mode == 1 ? "1" : "0");
}

static int modern_devfreq_match(const char *name) {
    const char *tokens[] = {"cpu-lat", "cpu_lat", "cpu-bw", "cpubw", "bw-hwmon", "bw_hwmon", "llcc", "l3", "memlat", "kgsl-ddr-qos", "bus_ddr", "bus_llcc", "devfreq_mif", "dvfsrc"};
    for (size_t i = 0; i < sizeof(tokens) / sizeof(tokens[0]); i++) {
        if (strstr(name, tokens[i])) return 1;
    }
    return 0;
}

static int modern_devfreq_cb(const char *entry_path, const char *entry_name, void *userdata) {
    int mode = *((int *)userdata);
    if (!modern_devfreq_match(entry_name)) return 0;
    if (mode == 1) return LITE_MODE == 1 ? devfreq_mid_perf(entry_path) : devfreq_max_perf(entry_path);
    if (mode == 2) return devfreq_unlock(entry_path);
    return devfreq_mid_perf(entry_path);
}

static void tune_modern_devfreq(int mode) {
    if (DEVICE_MITIGATION != 0) return;
    iterate_sysfs_nodes("/sys/class/devfreq", NULL, modern_devfreq_cb, &mode);
}

struct block_queue_data {
    const char *read_ahead;
    const char *requests;
};

static int block_queue_cb(const char *entry_path, const char *entry_name, void *userdata) {
    if (strncmp(entry_name, "loop", 4) == 0 || strncmp(entry_name, "ram", 3) == 0) return 0;
    struct block_queue_data *data = (struct block_queue_data *)userdata;
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s/queue/read_ahead_kb", entry_path);
    write_sysfs(path, data->read_ahead);
    snprintf(path, sizeof(path), "%s/queue/nr_requests", entry_path);
    write_sysfs(path, data->requests);
    return 1;
}

static void tune_block_queues(int mode) {
    struct block_queue_data data = {"128", "64"};
    if (mode == 1) {
        data.read_ahead = "32";
        data.requests = "32";
    } else if (mode == 3) {
        data.read_ahead = "64";
        data.requests = "32";
    }
    iterate_sysfs_nodes("/sys/block", NULL, block_queue_cb, &data);
}

static int scheduler_cb(const char *entry_path, const char *entry_name, void *userdata) {
    if (strncmp(entry_name, "loop", 4) == 0 || strncmp(entry_name, "ram", 3) == 0) return 0;
    const char **targets = (const char **)userdata;
    char scheduler_path[MAX_PATH_LEN];
    snprintf(scheduler_path, sizeof(scheduler_path), "%s/queue/scheduler", entry_path);
    for (int i = 0; targets[i]; i++) {
        char matched[64];
        if (sysfs_value_supported(scheduler_path, targets[i], matched, sizeof(matched))) {
            return write_sysfs(scheduler_path, matched[0] ? matched : targets[i]);
        }
    }
    return 0;
}

static void set_io_scheduler_mode(int mode) {
    const char *esport_targets[] = {"deadline", "zen", NULL};
    const char *balanced_targets[] = {"cfq", NULL};
    const char *efficiency_targets[] = {"noop", NULL};
    const char **targets = balanced_targets;
    if (mode == 1) targets = esport_targets;
    else if (mode == 3) targets = efficiency_targets;
    iterate_sysfs_nodes("/sys/block", NULL, scheduler_cb, targets);
}
