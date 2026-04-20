/*
 * modes_utils.c — Performance profile conductor for Velfox Profiler
 *
 * This is the sole orchestration layer for the three performance profiles.
 * It holds no tuning logic of its own; instead it:
 *   1. Reads runtime configuration from the module config directory.
 *   2. Calls memory_utils functions for VM, scheduler, and memory-bus tuning.
 *   3. Calls network_utils functions for TCP and RPS/XPS steering.
 *   4. Dispatches to SoC-specific routines via velfox_soc.h.
 *   5. Invokes cpufreq helpers from velfox_cpufreq.h.
 *
 * Adding a new tuning knob: implement it in memory_utils.c or network_utils.c,
 * expose it via the appropriate header, and call it here from the relevant
 * mode function.  Never write directly to sysfs/procfs from this file.
 *
 * Standard: C23 / Clang
 */

#include "velfox_modes.h"
#include "velfox_memory.h"
#include "velfox_network.h"
#include "velfox_cpufreq.h"
#include "velfox_devfreq.h"

/* -------------------------------------------------------------------------
 * Global runtime state — populated once by read_configs()
 * ---------------------------------------------------------------------- */
int  SOC              = 0;
int  LITE_MODE        = 0;
int  DEVICE_MITIGATION = 0;
char DEFAULT_CPU_GOV[50]  = "schedutil";
char PPM_POLICY[512]      = "";

/* =========================================================================
 * Configuration reader
 * ====================================================================== */

void read_configs(void) {
    char path[MAX_PATH_LEN];

    snprintf(path, sizeof(path), "%s/soc_recognition", MODULE_CONFIG);
    SOC = read_int_from_file(path);

    snprintf(path, sizeof(path), "%s/lite_mode", MODULE_CONFIG);
    LITE_MODE = read_int_from_file(path);

    snprintf(path, sizeof(path), "%s/ppm_policies_mediatek", MODULE_CONFIG);
    read_string_from_file(PPM_POLICY, sizeof(PPM_POLICY), path);

    /* Custom governor overrides the default if present */
    snprintf(path, sizeof(path), "%s/custom_default_cpu_gov", MODULE_CONFIG);
    if (file_exists(path)) {
        read_string_from_file(DEFAULT_CPU_GOV, sizeof(DEFAULT_CPU_GOV), path);
    } else {
        snprintf(path, sizeof(path), "%s/default_cpu_gov", MODULE_CONFIG);
        read_string_from_file(DEFAULT_CPU_GOV, sizeof(DEFAULT_CPU_GOV), path);
    }

    /* Ensure a sane fallback governor is always set */
    if (!DEFAULT_CPU_GOV[0]) {
        snprintf(DEFAULT_CPU_GOV, sizeof(DEFAULT_CPU_GOV), "schedutil");
    }

    snprintf(path, sizeof(path), "%s/device_mitigation", MODULE_CONFIG);
    DEVICE_MITIGATION = read_int_from_file(path);
}

/* =========================================================================
 * SoC dispatcher — private
 * ====================================================================== */

static void run_soc_mode(int mode) {
    switch (SOC) {
        case 1:
            if      (mode == 1) mediatek_esport();
            else if (mode == 2) mediatek_balanced();
            else                mediatek_efficiency();
            break;
        case 2:
            if      (mode == 1) snapdragon_esport();
            else if (mode == 2) snapdragon_balanced();
            else                snapdragon_efficiency();
            break;
        case 3:
            if      (mode == 1) exynos_esport();
            else if (mode == 2) exynos_balanced();
            else                exynos_efficiency();
            break;
        case 4:
            if      (mode == 1) unisoc_esport();
            else if (mode == 2) unisoc_balanced();
            else                unisoc_efficiency();
            break;
        case 5:
            if      (mode == 1) tensor_esport();
            else if (mode == 2) tensor_balanced();
            else                tensor_efficiency();
            break;
        default:
            /* Unknown SoC — no SoC-specific action */
            break;
    }
}

/* =========================================================================
 * DND helper — private
 * ====================================================================== */

void set_dnd(int mode) {
    if      (mode == 0) system("cmd notification set_dnd off");
    else if (mode == 1) system("cmd notification set_dnd priority");
}

static void maybe_set_dnd_from_config(int enabled) {
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s/dnd_gameplay", MODULE_CONFIG);
    if (read_int_from_file(path) == 1) set_dnd(enabled);
}

/* =========================================================================
 * Common touchpanel helper — private
 * ====================================================================== */

static void set_touchpanel_game_mode(int gaming) {
    const char *limit_val = gaming ? "0" : "1";
    const char *dir_val   = gaming ? "1" : "0";
    write_sysfs(PATH_TP_GAME_SWITCH,      gaming ? "1" : "0");
    write_sysfs(PATH_TP_OPLUS_LIMIT_ENABLE, limit_val);
    write_sysfs(PATH_TP_OPPO_LIMIT_ENABLE,  limit_val);
    write_sysfs(PATH_TP_OPLUS_DIRECTION,    dir_val);
    write_sysfs(PATH_TP_OPPO_DIRECTION,     dir_val);
}

/* =========================================================================
 * Block common baseline — private (called from perfcommon)
 * ====================================================================== */

static int block_common_cb(const char *entry_path,
                            const char *entry_name,
                            void       *userdata) {
    (void)userdata;
    if (strncmp(entry_name, "loop", 4) == 0 ||
        strncmp(entry_name, "ram",  3) == 0) {
        return 0;
    }
    char path[MAX_PATH_LEN];
    if (snprintf(path, sizeof(path), "%s/%s",
                 entry_path, LEAF_QUEUE_IOSTATS) < (int)sizeof(path)) {
        write_sysfs(path, "0");
    }
    if (snprintf(path, sizeof(path), "%s/%s",
                 entry_path, LEAF_QUEUE_ADD_RANDOM) < (int)sizeof(path)) {
        write_sysfs(path, "0");
    }
    return 1;
}

static int thermal_policy_cb(const char *entry_path,
                              const char *entry_name,
                              void       *userdata) {
    (void)entry_name;
    (void)userdata;
    char path[MAX_PATH_LEN];
    if (!path_join(path, sizeof(path), entry_path, LEAF_THERMAL_POLICY)) return 0;
    return write_sysfs(path, "step_wise");
}

/* =========================================================================
 * perfcommon — one-time baseline tweaks
 * ====================================================================== */

void perfcommon(void) {
    /* Kernel panic behavior — avoid reboots on recoverable errors */
    write_sysfs(PATH_KERNEL_PANIC,            "0");
    write_sysfs(PATH_KERNEL_PANIC_ON_OOPS,    "0");
    write_sysfs(PATH_KERNEL_PANIC_ON_WARN,    "0");
    write_sysfs(PATH_KERNEL_SOFTLOCKUP_PANIC, "0");

    /* Flush pending I/O before applying block-device tweaks */
    sync();

    /* Disable per-block noise and entropy contribution on all real devices */
    iterate_sysfs_nodes(PATH_SYSFS_BLOCK, NULL, block_common_cb, NULL);

    /* TCP baseline — congestion control, SACK, ECN, Fast Open */
    network_tune_tcp_baseline();

    /* Scheduler tuning — reduce overhead from stats and tracing */
    write_sysfs(PATH_PERF_CPU_TIME_MAX_PCT,  "3");
    write_sysfs(PATH_SCHED_SCHEDSTATS,       "0");
    write_sysfs(PATH_SCHED_AUTOGROUP_ENABLED,"0");
    write_sysfs(PATH_SCHED_CHILD_RUNS_FIRST, "1");
    write_sysfs(PATH_TASK_CPUSTATS_ENABLE,   "0");

    /* OEM vendor node suppressions */
    write_sysfs(PATH_MMC_USE_SPI_CRC,        "0");
    write_sysfs(PATH_OPCHAIN_ENABLE,          "0");
    write_sysfs(PATH_CPUFREQ_BOUNCING_ENABLE, "0");
    write_sysfs(PATH_TASK_SCHED_INFO_ENABLE,  "0");
    write_sysfs(PATH_OPLUS_SCHED_ASSIST,      "0");
    write_sysfs(PATH_KERNEL_PRINTK,           "0 0 0 0");

    /* cpuset memory spread baseline — background groups default off */
    write_sysfs(PATH_CPUSET_BG_MEM_SPREAD,   "0");
    write_sysfs(PATH_CPUSET_SYSBG_MEM_SPREAD,"0");

    /* VM baseline knobs that apply universally */
    write_sysfs(PATH_VM_PAGE_CLUSTER,            "0");
    write_sysfs(PATH_VM_STAT_INTERVAL,           "15");
    write_sysfs(PATH_VM_OVERCOMMIT_RATIO,        "80");
    write_sysfs(PATH_VM_EXTFRAG_THRESHOLD,       "640");
    write_sysfs(PATH_VM_WATERMARK_SCALE_FACTOR,  "22");

    /* Game engine library hints for the CFS scheduler */
    write_sysfs(PATH_SCHED_LIB_NAME,
        "libunity.so, libil2cpp.so, libmain.so, libUE4.so, "
        "libgodot_android.so, libgdx.so, libgdx-box2d.so, "
        "libminecraftpe.so, libLive2DCubismCore.so, "
        "libyuzu-android.so, libryujinx.so, libcitra-android.so, "
        "libhdr_pro_engine.so, libandroidx.graphics.path.so, "
        "libeffect.so");
    write_sysfs(PATH_SCHED_LIB_MASK_FORCE, "255");

    /* Set all thermal zones to a conservative baseline policy */
    iterate_sysfs_nodes(PATH_SYSFS_THERMAL, "thermal_zone",
                        thermal_policy_cb, NULL);

    /* Enable MGLRU reclaim if supported by this kernel */
    memory_enable_mglru();
}

/* =========================================================================
 * esport_mode — maximum performance
 * ====================================================================== */

void esport_mode(void) {
    maybe_set_dnd_from_config(1);

    /* Network */
    network_tune_workqueue(1);
    network_tune_rps_xps(1);
    network_tune_tcp_profile(1);

    /* Scheduler knobs */
    write_sysfs(PATH_SPLIT_LOCK_MITIGATE,           "0");
    write_sysfs(PATH_SCHED_ENERGY_AWARE,             "0");
    write_sysfs(PATH_SCHED_BOOST,                    "1");
    write_sysfs(PATH_STUNE_TOPAPP_PREFER_IDLE,       "1");
    write_sysfs(PATH_STUNE_TOPAPP_BOOST,             "1");
    write_sysfs(PATH_SCHED_NR_MIGRATE,               "32");
    write_sysfs(PATH_SCHED_RR_TIMESLICE_MS,          "4");
    write_sysfs(PATH_SCHED_TIME_AVG_MS,              "80");
    write_sysfs(PATH_SCHED_TUNABLE_SCALING,          "1");
    write_sysfs(PATH_SCHED_MIN_TASK_UTIL_BOOST,      "15");
    write_sysfs(PATH_SCHED_MIN_TASK_UTIL_COLOC,      "8");
    write_sysfs(PATH_SCHED_MIGRATION_COST_NS,        "50000");
    write_sysfs(PATH_SCHED_MIN_GRANULARITY_NS,       "800000");
    write_sysfs(PATH_SCHED_WAKEUP_GRANULARITY_NS,    "900000");
    write_sysfs(PATH_SCHED_FEATURES,                 "NEXT_BUDDY");
    write_sysfs(PATH_SCHED_FEATURES,                 "NO_TTWU_QUEUE");

    /* cpuset memory spread — enable for top-app and foreground */
    write_sysfs(PATH_CPUSET_TOPAPP_MEM_SPREAD, "1");
    write_sysfs(PATH_CPUSET_FGND_MEM_SPREAD,   "1");

    /* Touchpanel */
    set_touchpanel_game_mode(1);

    /* CPU governor and frequency */
    if (LITE_MODE == 0 && DEVICE_MITIGATION == 0) {
        change_cpu_gov("performance");
    } else {
        change_cpu_gov(DEFAULT_CPU_GOV);
    }
    if (file_exists(PATH_PROC_PPM)) cpufreq_ppm_max_perf();
    else                            cpufreq_max_perf();

    /* Memory subsystem */
    memory_tune_uclamp_profile(1);
    memory_tune_input_boost(1);
    memory_tune_devfreq(1);
    memory_tune_block_queues(1);
    memory_set_io_scheduler(1);
    memory_tune_vm_profile(1);

    /* SoC-specific dispatch */
    run_soc_mode(1);
}

/* =========================================================================
 * balanced_mode — everyday performance
 * ====================================================================== */

void balanced_mode(void) {
    maybe_set_dnd_from_config(0);

    /* Network */
    network_tune_workqueue(2);
    network_tune_rps_xps(2);
    network_tune_tcp_profile(2);

    /* Scheduler knobs */
    write_sysfs(PATH_SPLIT_LOCK_MITIGATE,           "1");
    write_sysfs(PATH_SCHED_ENERGY_AWARE,             "1");
    write_sysfs(PATH_SCHED_BOOST,                    "0");
    write_sysfs(PATH_STUNE_TOPAPP_PREFER_IDLE,       "0");
    write_sysfs(PATH_STUNE_TOPAPP_BOOST,             "1");
    write_sysfs(PATH_SCHED_RR_TIMESLICE_MS,          "6");
    write_sysfs(PATH_SCHED_TIME_AVG_MS,              "100");
    write_sysfs(PATH_SCHED_TUNABLE_SCALING,          "1");
    write_sysfs(PATH_SCHED_NR_MIGRATE,               "16");
    write_sysfs(PATH_SCHED_MIN_TASK_UTIL_BOOST,      "25");
    write_sysfs(PATH_SCHED_MIN_TASK_UTIL_COLOC,      "15");
    write_sysfs(PATH_SCHED_MIGRATION_COST_NS,        "100000");
    write_sysfs(PATH_SCHED_MIN_GRANULARITY_NS,       "1200000");
    write_sysfs(PATH_SCHED_WAKEUP_GRANULARITY_NS,    "2000000");
    write_sysfs(PATH_SCHED_FEATURES,                 "NEXT_BUDDY");
    write_sysfs(PATH_SCHED_FEATURES,                 "TTWU_QUEUE");

    /* cpuset memory spread — top-app on, foreground off */
    write_sysfs(PATH_CPUSET_TOPAPP_MEM_SPREAD, "1");
    write_sysfs(PATH_CPUSET_FGND_MEM_SPREAD,   "0");

    /* Touchpanel */
    set_touchpanel_game_mode(0);

    /* CPU governor and frequency */
    change_cpu_gov(DEFAULT_CPU_GOV);
    if (file_exists(PATH_PROC_PPM)) cpufreq_ppm_unlock();
    else                            cpufreq_unlock();

    /* Memory subsystem */
    memory_tune_uclamp_profile(2);
    memory_tune_input_boost(2);
    memory_tune_devfreq(2);
    memory_tune_block_queues(2);
    memory_set_io_scheduler(2);
    memory_tune_vm_profile(2);

    /* SoC-specific dispatch */
    run_soc_mode(2);
}

/* =========================================================================
 * efficiency_mode — power-saving profile
 * ====================================================================== */

void efficiency_mode(void) {
    maybe_set_dnd_from_config(0);

    /* Network */
    network_tune_workqueue(3);
    network_tune_rps_xps(3);
    network_tune_tcp_profile(3);

    /* Scheduler knobs */
    write_sysfs(PATH_SPLIT_LOCK_MITIGATE,           "1");
    write_sysfs(PATH_SCHED_ENERGY_AWARE,             "1");
    write_sysfs(PATH_SCHED_BOOST,                    "0");
    write_sysfs(PATH_STUNE_TOPAPP_PREFER_IDLE,       "1");
    write_sysfs(PATH_STUNE_TOPAPP_BOOST,             "0");
    write_sysfs(PATH_SCHED_RR_TIMESLICE_MS,          "8");
    write_sysfs(PATH_SCHED_TIME_AVG_MS,              "120");
    write_sysfs(PATH_SCHED_TUNABLE_SCALING,          "1");
    write_sysfs(PATH_SCHED_NR_MIGRATE,               "8");
    write_sysfs(PATH_SCHED_MIN_TASK_UTIL_BOOST,      "35");
    write_sysfs(PATH_SCHED_MIN_TASK_UTIL_COLOC,      "22");
    write_sysfs(PATH_SCHED_MIGRATION_COST_NS,        "180000");
    write_sysfs(PATH_SCHED_MIN_GRANULARITY_NS,       "1800000");
    write_sysfs(PATH_SCHED_WAKEUP_GRANULARITY_NS,    "2600000");
    write_sysfs(PATH_SCHED_FEATURES,                 "NO_NEXT_BUDDY");
    write_sysfs(PATH_SCHED_FEATURES,                 "TTWU_QUEUE");

    /* cpuset memory spread — all off in efficiency */
    write_sysfs(PATH_CPUSET_TOPAPP_MEM_SPREAD, "0");
    write_sysfs(PATH_CPUSET_FGND_MEM_SPREAD,   "0");

    /* Touchpanel */
    set_touchpanel_game_mode(0);

    /* CPU governor — allow a custom efficiency governor */
    char efficiency_gov_path[MAX_PATH_LEN];
    char efficiency_gov[50];
    snprintf(efficiency_gov_path, sizeof(efficiency_gov_path),
             "%s/efficiency_cpu_gov", MODULE_CONFIG);
    read_string_from_file(efficiency_gov, sizeof(efficiency_gov),
                          efficiency_gov_path);
    change_cpu_gov(efficiency_gov[0] ? efficiency_gov : DEFAULT_CPU_GOV);

    if (file_exists(PATH_PROC_PPM)) cpufreq_ppm_unlock();
    else                            cpufreq_unlock();

    /* Memory subsystem */
    memory_tune_uclamp_profile(3);
    memory_tune_input_boost(3);
    memory_tune_devfreq(3);
    memory_tune_block_queues(3);
    memory_set_io_scheduler(3);
    memory_tune_vm_profile(3);

    /* SoC-specific dispatch */
    run_soc_mode(3);
}
