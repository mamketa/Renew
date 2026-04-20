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

#include "scheduler_utils.h"
#include "utility_utils.h"

/* Common scheduler baseline */
void sched_common(void) {
    /* Perf event overhead cap */
    apply("3", "/proc/sys/kernel/perf_cpu_time_max_percent");

    /* Disable schedstats (reduces overhead) */
    apply("0", "/proc/sys/kernel/sched_schedstats");

    /* Disable autogroup — velfox handles priority manually */
    apply("0", "/proc/sys/kernel/sched_autogroup_enabled");

    /* Child runs first after fork — reduces wakeup latency */
    apply("1", "/proc/sys/kernel/sched_child_runs_first");

    /* Disable Oppo/Realme cpustats */
    apply("0", "/proc/sys/kernel/task_cpustats_enable");

    /* Disable Oplus scheduler bloats */
    apply("0", "/sys/module/cpufreq_bouncing/parameters/enable");
    apply("0", "/proc/task_info/task_sched_info/task_sched_info_enable");
    apply("0", "/proc/oplus_scheduler/sched_assist/sched_assist_enabled");

    /* Report max CPU capabilities for game engines */
    apply("libunity.so, libil2cpp.so, libmain.so, libUE4.so, libgodot_android.so, "
          "libgdx.so, libgdx-box2d.so, libminecraftpe.so, libLive2DCubismCore.so, "
          "libyuzu-android.so, libryujinx.so, libcitra-android.so, libhdr_pro_engine.so, "
          "libandroidx.graphics.path.so, libeffect.so",
          "/proc/sys/kernel/sched_lib_name");
    apply("255", "/proc/sys/kernel/sched_lib_mask_force");
}

void sched_tune_esport(void) {
    /* Energy-aware scheduling OFF — let all cores run at peak */
    apply("0", "/proc/sys/kernel/sched_energy_aware");

    /* Sched boost */
    apply("1", "/proc/sys/kernel/sched_boost");
    apply("1", "/dev/stune/top-app/schedtune.prefer_idle");
    apply("1", "/dev/stune/top-app/schedtune.boost");

    /* Improve real-time latencies */
    apply("32",     "/proc/sys/kernel/sched_nr_migrate");
    apply("4",      "/proc/sys/kernel/sched_rr_timeslice_ms");
    apply("80",     "/proc/sys/kernel/sched_time_avg_ms");
    apply("1",      "/proc/sys/kernel/sched_tunable_scaling");

    /* Scheduler tuning */
    apply("15",     "/proc/sys/kernel/sched_min_task_util_for_boost");
    apply("8",      "/proc/sys/kernel/sched_min_task_util_for_colocation");
    apply("50000",  "/proc/sys/kernel/sched_migration_cost_ns");
    apply("800000", "/proc/sys/kernel/sched_min_granularity_ns");
    apply("900000", "/proc/sys/kernel/sched_wakeup_granularity_ns");

    /* Sched features */
    apply("NEXT_BUDDY",   "/sys/kernel/debug/sched_features");
    apply("NO_TTWU_QUEUE", "/sys/kernel/debug/sched_features");

    /* CPU shares */
    apply("768", "/dev/cpuctl/top-app/cpu.shares");
    apply("512", "/dev/cpuctl/foreground/cpu.shares");
    apply("64",  "/dev/cpuctl/background/cpu.shares");
    apply("96",  "/dev/cpuctl/system-background/cpu.shares");
    apply("192", "/dev/cpuctl/nnapi-hal/cpu.shares");
    apply("192", "/dev/cpuctl/dex2oat/cpu.shares");
    apply("512", "/dev/cpuctl/system/cpu.shares");

    /* cpuset memory spread */
    apply("1", "/dev/cpuset/top-app/memory_spread_page");
    apply("1", "/dev/cpuset/foreground/memory_spread_page");

    /* uclamp — cap background, guarantee top-app */
    apply("128", "/dev/cpuctl/background/cpu.uclamp.max");
    apply("128", "/dev/cpuctl/system-background/cpu.uclamp.max");
    apply("128", "/dev/cpuctl/restricted-background/cpu.uclamp.max");
    apply("256", "/sys/fs/cgroup/uclamp/top-app.uclamp.min");
    apply("128", "/sys/fs/cgroup/uclamp/foreground.uclamp.min");
    apply("32",  "/sys/fs/cgroup/uclamp/background.uclamp.min");

    /* Disable split lock mitigation */
    apply("0", "/proc/sys/kernel/split_lock_mitigate");

    /* Disable OnePlus opchain */
    apply("0", "/sys/module/opchain/parameters/chain_on");
}

void sched_tune_balanced(void) {
    /* Re-enable EAS for balanced power/perf trade-off */
    apply("1", "/proc/sys/kernel/sched_energy_aware");

    apply("0", "/proc/sys/kernel/sched_boost");
    apply("0", "/dev/stune/top-app/schedtune.prefer_idle");
    apply("0", "/dev/stune/top-app/schedtune.boost");

    apply("32",      "/proc/sys/kernel/sched_nr_migrate");
    apply("10",      "/proc/sys/kernel/sched_rr_timeslice_ms");
    apply("1000000", "/proc/sys/kernel/sched_min_granularity_ns");
    apply("1500000", "/proc/sys/kernel/sched_wakeup_granularity_ns");

    /* Balanced CPU shares */
    apply("512", "/dev/cpuctl/top-app/cpu.shares");
    apply("256", "/dev/cpuctl/foreground/cpu.shares");
    apply("64",  "/dev/cpuctl/background/cpu.shares");
    apply("96",  "/dev/cpuctl/system-background/cpu.shares");
    apply("512", "/dev/cpuctl/system/cpu.shares");

    apply("0", "/dev/cpuset/top-app/memory_spread_page");
    apply("0", "/dev/cpuset/foreground/memory_spread_page");
}

void sched_tune_efficiency(void) {
    /* Keep EAS active for maximum power savings */
    apply("1", "/proc/sys/kernel/sched_energy_aware");

    apply("0", "/proc/sys/kernel/sched_boost");
    apply("0", "/dev/stune/top-app/schedtune.prefer_idle");
    apply("0", "/dev/stune/top-app/schedtune.boost");

    apply("8",       "/proc/sys/kernel/sched_nr_migrate");
    apply("20",      "/proc/sys/kernel/sched_rr_timeslice_ms");
    apply("2000000", "/proc/sys/kernel/sched_min_granularity_ns");
    apply("2500000", "/proc/sys/kernel/sched_wakeup_granularity_ns");

    /* Efficiency: restrict top-app shares moderately, heavily limit background */
    apply("256", "/dev/cpuctl/top-app/cpu.shares");
    apply("128", "/dev/cpuctl/foreground/cpu.shares");
    apply("32",  "/dev/cpuctl/background/cpu.shares");
    apply("48",  "/dev/cpuctl/system-background/cpu.shares");
    apply("256", "/dev/cpuctl/system/cpu.shares");

    /* uclamp: cap foreground too for efficiency */
    apply("512", "/dev/cpuctl/top-app/cpu.uclamp.max");
    apply("256", "/dev/cpuctl/foreground/cpu.uclamp.max");
    apply("64",  "/dev/cpuctl/background/cpu.uclamp.max");
    apply("64",  "/dev/cpuctl/system-background/cpu.uclamp.max");

    apply("0", "/dev/cpuset/top-app/memory_spread_page");
    apply("0", "/dev/cpuset/foreground/memory_spread_page");
}
