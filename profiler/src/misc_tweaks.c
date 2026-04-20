/*
 * Copyright (C) 2025-2026 VelocityFox22
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#include "velfox_common.h"
#include "velfox_devfreq.h"
#include "velfox_misc.h"

void sync(void);

static bool any_name(const char *name, void *ctx) {
    (void)ctx;
    return name && name[0] != '.';
}

void set_dnd(int mode) {
    if (mode == 0) system("cmd notification set_dnd off");
    else if (mode == 1) system("cmd notification set_dnd priority");
}

typedef struct {
    const char *mask;
} queue_ctx;

static bool queue_name(const char *name, void *ctx) {
    (void)ctx;
    return name_starts_with(name, "rx-") || name_starts_with(name, "tx-");
}

static void queue_handler(const char *dir, const char *name, void *ctx) {
    queue_ctx *data = ctx;
    char path[MAX_PATH_LEN];
    const char *leaf = name_starts_with(name, "rx-") ? "rps_cpus" : "xps_cpus";
    if (path_join3(path, sizeof(path), dir, name, leaf)) apply(data->mask, path);
}

static void iface_handler(const char *dir, const char *name, void *ctx) {
    char queue_dir[MAX_PATH_LEN];
    if (!path_join3(queue_dir, sizeof(queue_dir), dir, name, "queues")) return;
    for_each_dir_entry(queue_dir, queue_name, queue_handler, ctx);
}

void set_rps_xps(int mode) {
    int cores = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (cores <= 0) return;
    int use = cores;
    if (mode == 2) use = (cores * 3) / 4;
    else if (mode == 3) use = cores / 2;
    if (use < 1) use = 1;
    if (use > (int)(sizeof(unsigned long) * 8)) use = (int)(sizeof(unsigned long) * 8);
    unsigned long mask = 0;
    for (int i = 0; i < use; ++i) mask |= 1UL << i;
    char mask_hex[32];
    snprintf(mask_hex, sizeof(mask_hex), "%lx", mask);
    queue_ctx ctx = {.mask = mask_hex};
    for_each_dir_entry("/sys/class/net", any_name, iface_handler, &ctx);
}

static void common_block_handler(const char *dir, const char *name, void *ctx) {
    (void)ctx;
    char path[MAX_PATH_LEN];
    if (path_join3(path, sizeof(path), dir, name, "queue/iostats")) apply("0", path);
    if (path_join3(path, sizeof(path), dir, name, "queue/add_random")) apply("0", path);
}

static void thermal_handler(const char *dir, const char *name, void *ctx) {
    (void)ctx;
    char path[MAX_PATH_LEN];
    if (path_join3(path, sizeof(path), dir, name, "policy")) apply("step_wise", path);
}

void misc_common_tweaks(void) {
    apply("0", "/proc/sys/kernel/panic");
    apply("0", "/proc/sys/kernel/panic_on_oops");
    apply("0", "/proc/sys/kernel/panic_on_warn");
    apply("0", "/proc/sys/kernel/softlockup_panic");
    sync();
    for_each_dir_entry("/sys/block", any_name, common_block_handler, NULL);
    const char *algorithms[] = {"bbr3", "bbr2", "bbrplus", "bbr", "westwood", "cubic"};
    for (size_t i = 0; i < sizeof(algorithms) / sizeof(algorithms[0]); ++i) {
        char cmd[MAX_PATH_LEN];
        snprintf(cmd, sizeof(cmd), "grep -q \"%s\" /proc/sys/net/ipv4/tcp_available_congestion_control", algorithms[i]);
        if (system(cmd) == 0) {
            apply(algorithms[i], "/proc/sys/net/ipv4/tcp_congestion_control");
            break;
        }
    }
    apply("1", "/proc/sys/net/ipv4/tcp_sack");
    apply("1", "/proc/sys/net/ipv4/tcp_ecn");
    apply("1", "/proc/sys/net/ipv4/tcp_window_scaling");
    apply("1", "/proc/sys/net/ipv4/tcp_moderate_rcvbuf");
    apply("3", "/proc/sys/net/ipv4/tcp_fastopen");
    apply("3", "/proc/sys/kernel/perf_cpu_time_max_percent");
    apply("0", "/proc/sys/kernel/sched_schedstats");
    apply("0", "/proc/sys/kernel/sched_autogroup_enabled");
    apply("1", "/proc/sys/kernel/sched_child_runs_first");
    apply("0", "/proc/sys/kernel/task_cpustats_enable");
    apply("0", "/sys/module/mmc_core/parameters/use_spi_crc");
    apply("0", "/sys/module/opchain/parameters/chain_on");
    apply("0", "/sys/module/cpufreq_bouncing/parameters/enable");
    apply("0", "/proc/task_info/task_sched_info/task_sched_info_enable");
    apply("0", "/proc/oplus_scheduler/sched_assist/sched_assist_enabled");
    apply("0 0 0 0", "/proc/sys/kernel/printk");
    apply("0", "/dev/cpuset/background/memory_spread_page");
    apply("0", "/dev/cpuset/system-background/memory_spread_page");
    apply("0", "/proc/sys/vm/page-cluster");
    apply("15", "/proc/sys/vm/stat_interval");
    apply("80", "/proc/sys/vm/overcommit_ratio");
    apply("640", "/proc/sys/vm/extfrag_threshold");
    apply("22", "/proc/sys/vm/watermark_scale_factor");
    apply("libunity.so, libil2cpp.so, libmain.so, libUE4.so, libgodot_android.so, libgdx.so, libgdx-box2d.so, libminecraftpe.so, libLive2DCubismCore.so, libyuzu-android.so, libryujinx.so, libcitra-android.so, libhdr_pro_engine.so, libandroidx.graphics.path.so, libeffect.so", "/proc/sys/kernel/sched_lib_name");
    apply("255", "/proc/sys/kernel/sched_lib_mask_force");
    const char *thermal[] = {"thermal_zone", NULL};
    for_each_dir_entry("/sys/class/thermal", name_has_token, thermal_handler, (void *)thermal);
}

void misc_mode_sched_tweaks(int mode) {
    if (mode == 1) {
        apply("0", "/proc/sys/net/ipv4/tcp_slow_start_after_idle");
        apply("0", "/proc/sys/kernel/split_lock_mitigate");
        apply("0", "/proc/sys/kernel/sched_energy_aware");
        apply("1", "/proc/sys/kernel/sched_boost");
        apply("1", "/dev/stune/top-app/schedtune.prefer_idle");
        apply("1", "/dev/stune/top-app/schedtune.boost");
        apply("32", "/proc/sys/kernel/sched_nr_migrate");
        apply("4", "/proc/sys/kernel/sched_rr_timeslice_ms");
        apply("80", "/proc/sys/kernel/sched_time_avg_ms");
        apply("1", "/proc/sys/kernel/sched_tunable_scaling");
        apply("15", "/proc/sys/kernel/sched_min_task_util_for_boost");
        apply("8", "/proc/sys/kernel/sched_min_task_util_for_colocation");
        apply("50000", "/proc/sys/kernel/sched_migration_cost_ns");
        apply("800000", "/proc/sys/kernel/sched_min_granularity_ns");
        apply("900000", "/proc/sys/kernel/sched_wakeup_granularity_ns");
        apply("NEXT_BUDDY", "/sys/kernel/debug/sched_features");
        apply("NO_TTWU_QUEUE", "/sys/kernel/debug/sched_features");
        apply("768", "/dev/cpuctl/top-app/cpu.shares");
        apply("512", "/dev/cpuctl/foreground/cpu.shares");
        apply("1", "/dev/cpuset/top-app/memory_spread_page");
        apply("1", "/dev/cpuset/foreground/memory_spread_page");
        apply("64", "/dev/cpuctl/background/cpu.shares");
        apply("96", "/dev/cpuctl/system-background/cpu.shares");
        apply("192", "/dev/cpuctl/nnapi-hal/cpu.shares");
        apply("192", "/dev/cpuctl/dex2oat/cpu.shares");
        apply("512", "/dev/cpuctl/system/cpu.shares");
        apply("128", "/dev/cpuctl/background/cpu.uclamp.max");
        apply("128", "/dev/cpuctl/system-background/cpu.uclamp.max");
        apply("128", "/dev/cpuctl/restricted-background/cpu.uclamp.max");
        apply("256", "/sys/fs/cgroup/uclamp/top-app.uclamp.min");
        apply("128", "/sys/fs/cgroup/uclamp/foreground.uclamp.min");
        apply("32", "/sys/fs/cgroup/uclamp/background.uclamp.min");
    } else if (mode == 2) {
        apply("1", "/proc/sys/net/ipv4/tcp_slow_start_after_idle");
        apply("1", "/proc/sys/kernel/split_lock_mitigate");
        apply("1", "/proc/sys/kernel/sched_energy_aware");
        apply("0", "/proc/sys/kernel/sched_boost");
        apply("0", "/dev/stune/top-app/schedtune.prefer_idle");
        apply("1", "/dev/stune/top-app/schedtune.boost");
        apply("6", "/proc/sys/kernel/sched_rr_timeslice_ms");
        apply("100", "/proc/sys/kernel/sched_time_avg_ms");
        apply("1", "/proc/sys/kernel/sched_tunable_scaling");
        apply("16", "/proc/sys/kernel/sched_nr_migrate");
        apply("25", "/proc/sys/kernel/sched_min_task_util_for_boost");
        apply("15", "/proc/sys/kernel/sched_min_task_util_for_colocation");
        apply("100000", "/proc/sys/kernel/sched_migration_cost_ns");
        apply("1200000", "/proc/sys/kernel/sched_min_granularity_ns");
        apply("2000000", "/proc/sys/kernel/sched_wakeup_granularity_ns");
        apply("NEXT_BUDDY", "/sys/kernel/debug/sched_features");
        apply("TTWU_QUEUE", "/sys/kernel/debug/sched_features");
        apply("512", "/dev/cpuctl/top-app/cpu.shares");
        apply("384", "/dev/cpuctl/foreground/cpu.shares");
        apply("1", "/dev/cpuset/top-app/memory_spread_page");
        apply("0", "/dev/cpuset/foreground/memory_spread_page");
        apply("128", "/dev/cpuctl/background/cpu.shares");
        apply("192", "/dev/cpuctl/system-background/cpu.shares");
        apply("192", "/dev/cpuctl/nnapi-hal/cpu.shares");
        apply("192", "/dev/cpuctl/dex2oat/cpu.shares");
        apply("512", "/dev/cpuctl/system/cpu.shares");
        apply("256", "/dev/cpuctl/background/cpu.uclamp.max");
        apply("256", "/dev/cpuctl/system-background/cpu.uclamp.max");
        apply("256", "/dev/cpuctl/restricted-background/cpu.uclamp.max");
        apply("192", "/sys/fs/cgroup/uclamp/top-app.uclamp.min");
        apply("96", "/sys/fs/cgroup/uclamp/foreground.uclamp.min");
        apply("32", "/sys/fs/cgroup/uclamp/background.uclamp.min");
    } else {
        apply("1", "/proc/sys/net/ipv4/tcp_slow_start_after_idle");
        apply("1", "/proc/sys/kernel/split_lock_mitigate");
        apply("1", "/proc/sys/kernel/sched_energy_aware");
        apply("0", "/proc/sys/kernel/sched_boost");
        apply("1", "/dev/stune/top-app/schedtune.prefer_idle");
        apply("0", "/dev/stune/top-app/schedtune.boost");
        apply("10", "/proc/sys/kernel/sched_rr_timeslice_ms");
        apply("150", "/proc/sys/kernel/sched_time_avg_ms");
        apply("1", "/proc/sys/kernel/sched_tunable_scaling");
        apply("8", "/proc/sys/kernel/sched_nr_migrate");
        apply("45", "/proc/sys/kernel/sched_min_task_util_for_boost");
        apply("30", "/proc/sys/kernel/sched_min_task_util_for_colocation");
        apply("200000", "/proc/sys/kernel/sched_migration_cost_ns");
        apply("2000000", "/proc/sys/kernel/sched_min_granularity_ns");
        apply("3000000", "/proc/sys/kernel/sched_wakeup_granularity_ns");
        apply("NO_NEXT_BUDDY", "/sys/kernel/debug/sched_features");
        apply("TTWU_QUEUE", "/sys/kernel/debug/sched_features");
        apply("0", "/dev/cpuset/top-app/memory_spread_page");
        apply("0", "/dev/cpuset/foreground/memory_spread_page");
        apply("384", "/dev/cpuctl/top-app/cpu.shares");
        apply("256", "/dev/cpuctl/foreground/cpu.shares");
        apply("128", "/dev/cpuctl/background/cpu.shares");
        apply("128", "/dev/cpuctl/system-background/cpu.shares");
        apply("128", "/dev/cpuctl/nnapi-hal/cpu.shares");
        apply("128", "/dev/cpuctl/dex2oat/cpu.shares");
        apply("384", "/dev/cpuctl/system/cpu.shares");
        apply("128", "/dev/cpuctl/background/cpu.uclamp.max");
        apply("128", "/dev/cpuctl/system-background/cpu.uclamp.max");
        apply("128", "/dev/cpuctl/restricted-background/cpu.uclamp.max");
        apply("128", "/sys/fs/cgroup/uclamp/top-app.uclamp.min");
        apply("64", "/sys/fs/cgroup/uclamp/foreground.uclamp.min");
        apply("16", "/sys/fs/cgroup/uclamp/background.uclamp.min");
    }
}

static void block_tune(const char *name, const char *read_ahead, const char *requests) {
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "/sys/block/%s/queue/read_ahead_kb", name);
    apply(read_ahead, path);
    snprintf(path, sizeof(path), "/sys/block/%s/queue/nr_requests", name);
    apply(requests, path);
}

static void scheduler_tune(const char *name, int mode) {
    char path[MAX_PATH_LEN];
    char available[256];
    snprintf(path, sizeof(path), "/sys/block/%s/queue/scheduler", name);
    read_string_from_file(available, sizeof(available), path);
    const char *const *candidates;
    static const char *balanced[] = {"cfq", "bfq", NULL};
    static const char *efficiency[] = {"noop", "none", NULL};
    static const char *esport[] = {"deadline", "mq-deadline", NULL};
    candidates = mode == 1 ? esport : mode == 2 ? balanced : efficiency;
    for (size_t i = 0; candidates[i]; ++i) {
        if (strstr(available, candidates[i])) {
            apply(candidates[i], path);
            break;
        }
    }
}

typedef struct { int mode; } io_ctx;

static bool sd_name(const char *name, void *ctx) {
    (void)ctx;
    return name_starts_with(name, "sd");
}

static bool storage_name(const char *name, void *ctx) {
    (void)ctx;
    return name_contains(name, ".ufshc") || name_contains(name, "mmc");
}

static void sd_block_handler(const char *dir, const char *name, void *ctx) {
    (void)dir;
    io_ctx *data = ctx;
    scheduler_tune(name, data->mode);
    if (data->mode == 1) block_tune(name, "32", "32");
    else block_tune(name, "128", "64");
}

void misc_mode_io_tweaks(int mode) {
    io_ctx ctx = {.mode = mode};
    scheduler_tune("mmcblk0", mode);
    scheduler_tune("mmcblk1", mode);
    if (mode == 1) {
        block_tune("mmcblk0", "32", "32");
        block_tune("mmcblk1", "32", "32");
    } else if (mode == 2) {
        block_tune("mmcblk0", "64", "64");
        block_tune("mmcblk1", "64", "64");
    } else {
        block_tune("mmcblk0", "16", "16");
        block_tune("mmcblk1", "16", "16");
    }
    for_each_dir_entry("/sys/block", sd_name, sd_block_handler, &ctx);
}

void misc_mode_vm_tweaks(int mode) {
    if (mode == 1) {
        apply("20", "/proc/sys/vm/swappiness");
        apply("60", "/proc/sys/vm/vfs_cache_pressure");
        apply("30", "/proc/sys/vm/compaction_proactiveness");
        apply("5", "/proc/sys/vm/dirty_background_ratio");
        apply("15", "/proc/sys/vm/dirty_ratio");
        apply("1500", "/proc/sys/vm/dirty_expire_centisecs");
        apply("1500", "/proc/sys/vm/dirty_writeback_centisecs");
    } else if (mode == 2) {
        apply("40", "/proc/sys/vm/swappiness");
        apply("80", "/proc/sys/vm/vfs_cache_pressure");
        apply("40", "/proc/sys/vm/compaction_proactiveness");
        apply("10", "/proc/sys/vm/dirty_background_ratio");
        apply("25", "/proc/sys/vm/dirty_ratio");
        apply("3000", "/proc/sys/vm/dirty_expire_centisecs");
        apply("3000", "/proc/sys/vm/dirty_writeback_centisecs");
    } else {
        apply("60", "/proc/sys/vm/swappiness");
        apply("100", "/proc/sys/vm/vfs_cache_pressure");
        apply("10", "/proc/sys/vm/compaction_proactiveness");
        apply("20", "/proc/sys/vm/dirty_background_ratio");
        apply("40", "/proc/sys/vm/dirty_ratio");
        apply("6000", "/proc/sys/vm/dirty_writeback_centisecs");
        apply("6000", "/proc/sys/vm/dirty_expire_centisecs");
    }
}

static void storage_devfreq_handler(const char *dir, const char *name, void *ctx) {
    int mode = *(int *)ctx;
    char path[MAX_PATH_LEN];
    if (!path_join(path, sizeof(path), dir, name)) return;
    if (mode == 1) LITE_MODE == 1 ? devfreq_mid_perf(path) : devfreq_max_perf(path);
    else if (mode == 2) devfreq_unlock(path);
    else devfreq_min_perf(path);
}

void misc_storage_devfreq_tweaks(int mode) {
    for_each_dir_entry("/sys/class/devfreq", storage_name, storage_devfreq_handler, &(int){mode});
}

static void schedutil_handler(const char *dir, const char *name, void *ctx) {
    int mode = *(int *)ctx;
    const char *rate = mode == 1 ? "500" : mode == 2 ? "2000" : "8000";
    const char *up = mode == 1 ? "500" : mode == 2 ? "2000" : "4000";
    const char *down = mode == 1 ? "10000" : mode == 2 ? "20000" : "30000";
    char base[MAX_PATH_LEN], path[MAX_PATH_LEN];
    if (!path_join3(base, sizeof(base), dir, name, "schedutil")) return;
    snprintf(path, sizeof(path), "%s/rate_limit_us", base);
    apply(rate, path);
    snprintf(path, sizeof(path), "%s/up_rate_limit_us", base);
    apply(up, path);
    snprintf(path, sizeof(path), "%s/down_rate_limit_us", base);
    apply(down, path);
}

void misc_universal_tweaks(int mode) {
    const char *top_min = mode == 1 ? "256" : mode == 2 ? "192" : "128";
    const char *bg_max = mode == 1 ? "128" : mode == 2 ? "256" : "192";
    for_each_dir_entry("/sys/devices/system/cpu/cpufreq", name_is_policy, schedutil_handler, &(int){mode});
    apply(top_min, "/dev/stune/top-app/uclamp.min");
    apply(bg_max, "/dev/stune/background/uclamp.max");
    apply(top_min, "/proc/sys/kernel/sched_util_clamp_min");
    apply(mode == 1 ? "1" : "0", "/proc/sys/kernel/sched_boost");
    apply(mode == 1 ? "1" : "0", "/sys/module/cpu_input_boost/parameters/enabled");
    apply(mode == 1 ? "1" : mode == 2 ? "1" : "0", "/sys/module/cpu_input_boost/parameters/enable");
    apply(mode == 1 ? "1200" : mode == 2 ? "800" : "400", "/sys/module/cpu_input_boost/parameters/input_boost_duration");
    apply(mode == 1 ? "1" : mode == 2 ? "0" : "0", "/sys/module/cpu_input_boost/parameters/dynamic_stune_boost");
}
