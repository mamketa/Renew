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

#include "velfox_soc.h"
#include "velfox_cpufreq.h"
#include "velfox_devfreq.h"

typedef struct { const char *state; } ppm_ctx;

static void ppm_policy_handler(const char *dir, const char *name, void *ctx) {
    (void)dir;
    (void)name;
    ppm_ctx *data = ctx;
    FILE *fp = fopen("/proc/ppm/policy_status", "r");
    if (!fp) return;
    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), fp)) {
        if (PPM_POLICY[0] && strstr(line, PPM_POLICY)) {
            char cmd[16];
            snprintf(cmd, sizeof(cmd), "%c %s", line[1], data->state);
            apply(cmd, "/proc/ppm/policy_status");
        }
    }
    fclose(fp);
}

static void mtk_ppm_policy(const char *state) {
    ppm_ctx ctx = {.state = state};
    ppm_policy_handler(NULL, NULL, &ctx);
}

static void mtk_fpsgo(int mode) {
    apply(mode == 1 ? "120" : "60", "/sys/kernel/fpsgo/fbt/fixed_target_fps");
    apply("1", "/sys/module/mtk_fpsgo/parameters/perfmgr_enable");
    if (mode == 1) {
        apply("0", "/sys/kernel/fpsgo/fstb/adopt_low_fps");
        apply("1", "/sys/pnpmgr/fpsgo_boost/boost_enable");
        apply("1", "/sys/kernel/fpsgo/fstb/gpu_slowdown_check");
    } else {
        apply("1", "/sys/kernel/fpsgo/fstb/adopt_low_fps");
        apply("0", "/sys/pnpmgr/fpsgo_boost/boost_enable");
        apply("0", "/sys/kernel/fpsgo/fstb/gpu_slowdown_check");
        apply("1", "/sys/kernel/fpsgo/fbt/enable_switch_down_throttle");
        if (mode == 2) {
            apply("1", "/sys/kernel/fpsgo/common/force_onoff");
            apply("1", "/sys/kernel/fpsgo/fstb/fstb_self_ctrl_fps_enable");
            apply("0", "/sys/kernel/fpsgo/fbt/enable_switch_down_throttle");
        }
    }
}

static long mtk_gpufreq_min_freq(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    char line[MAX_LINE_LEN];
    long min_freq = LONG_MAX;
    while (fgets(line, sizeof(line), fp)) {
        char *freq_str = strstr(line, "freq =");
        if (!freq_str) continue;
        long freq = atol(freq_str + 6);
        if (freq > 0 && freq < min_freq) min_freq = freq;
    }
    fclose(fp);
    return min_freq == LONG_MAX ? 0 : min_freq;
}

void mediatek_esport(void) {
    mtk_ppm_policy("0");
    apply("1", "/proc/ppm/mode");
    mtk_fpsgo(1);
    apply("1", "/proc/cpufreq/cpufreq_cci_mode");
    apply("3", "/proc/cpufreq/cpufreq_power_mode");
    apply("1", "/proc/perfmgr/syslimiter/syslimiter_force_disable");
    apply("1", "/sys/devices/platform/boot_dramboost/dramboost/dramboost");
    apply("0", "/sys/devices/system/cpu/eas/enable");
    if (LITE_MODE == 0) {
        if (file_exists("/proc/gpufreqv2")) apply("0", "/proc/gpufreqv2/fix_target_opp_index");
        else if (file_exists("/proc/gpufreq/gpufreq_opp_dump")) {
            FILE *fp = fopen("/proc/gpufreq/gpufreq_opp_dump", "r");
            if (fp) {
                char line[MAX_LINE_LEN];
                if (fgets(line, sizeof(line), fp)) {
                    char *freq_str = strstr(line, "freq =");
                    if (freq_str) apply_ll(atol(freq_str + 6), "/proc/gpufreq/gpufreq_opp_freq");
                }
                fclose(fp);
            }
        }
    } else {
        apply("0", "/proc/gpufreq/gpufreq_opp_freq");
        apply("-1", "/proc/gpufreqv2/fix_target_opp_index");
        int mid = file_exists("/proc/gpufreqv2/gpu_working_opp_table") ? mtk_gpufreq_midfreq_index("/proc/gpufreqv2/gpu_working_opp_table") : mtk_gpufreq_midfreq_index("/proc/gpufreq/gpufreq_opp_dump");
        apply_ll(mid, "/sys/kernel/ged/hal/custom_boost_gpu_freq");
    }
    apply("ignore_batt_oc 1", "/proc/gpufreq/gpufreq_power_limited");
    apply("ignore_batt_percent 1", "/proc/gpufreq/gpufreq_power_limited");
    apply("ignore_low_batt 1", "/proc/gpufreq/gpufreq_power_limited");
    apply("ignore_thermal_protect 1", "/proc/gpufreq/gpufreq_power_limited");
    apply("ignore_pbm_limited 1", "/proc/gpufreq/gpufreq_power_limited");
    apply("stop 1", "/proc/mtk_batoc_throttling/battery_oc_protect_stop");
    if (LITE_MODE == 0) {
        apply("0", "/sys/devices/platform/10012000.dvfsrc/helio-dvfsrc/dvfsrc_req_ddr_opp");
        apply("0", "/sys/kernel/helio-dvfsrc/dvfsrc_force_vcore_dvfs_opp");
        devfreq_max_perf("/sys/class/devfreq/mtk-dvfsrc-devfreq");
    } else {
        apply("-1", "/sys/devices/platform/10012000.dvfsrc/helio-dvfsrc/dvfsrc_req_ddr_opp");
        apply("-1", "/sys/kernel/helio-dvfsrc/dvfsrc_force_vcore_dvfs_opp");
        devfreq_mid_perf("/sys/class/devfreq/mtk-dvfsrc-devfreq");
    }
    apply("0", "/sys/kernel/eara_thermal/enable");
}

void mediatek_balanced(void) {
    mtk_ppm_policy("1");
    apply("0", "/proc/ppm/mode");
    mtk_fpsgo(2);
    apply("0", "/proc/cpufreq/cpufreq_cci_mode");
    apply("2", "/proc/cpufreq/cpufreq_power_mode");
    apply("0", "/proc/perfmgr/syslimiter/syslimiter_force_disable");
    apply("0", "/sys/devices/platform/boot_dramboost/dramboost/dramboost");
    apply("2", "/sys/devices/system/cpu/eas/enable");
    apply("0", "/sys/module/ged/parameters/is_GED_KPI_enabled");
    write_file("0", "/proc/gpufreq/gpufreq_opp_freq");
    write_file("-1", "/proc/gpufreqv2/fix_target_opp_index");
    apply("1", "/sys/module/ged/parameters/gpu_dvfs_enable");
    int min = file_exists("/proc/gpufreqv2/gpu_working_opp_table") ? mtk_gpufreq_minfreq_index("/proc/gpufreqv2/gpu_working_opp_table") : mtk_gpufreq_minfreq_index("/proc/gpufreq/gpufreq_opp_dump");
    apply_ll(min, "/sys/kernel/ged/hal/custom_boost_gpu_freq");
    apply("ignore_batt_oc 0", "/proc/gpufreq/gpufreq_power_limited");
    apply("ignore_batt_percent 0", "/proc/gpufreq/gpufreq_power_limited");
    apply("ignore_low_batt 0", "/proc/gpufreq/gpufreq_power_limited");
    apply("ignore_thermal_protect 0", "/proc/gpufreq/gpufreq_power_limited");
    apply("ignore_pbm_limited 0", "/proc/gpufreq/gpufreq_power_limited");
    apply("stop 0", "/proc/mtk_batoc_throttling/battery_oc_protect_stop");
    write_file("-1", "/sys/devices/platform/10012000.dvfsrc/helio-dvfsrc/dvfsrc_req_ddr_opp");
    write_file("-1", "/sys/kernel/helio-dvfsrc/dvfsrc_force_vcore_dvfs_opp");
    devfreq_unlock("/sys/class/devfreq/mtk-dvfsrc-devfreq");
    apply("1", "/sys/kernel/eara_thermal/enable");
}

void mediatek_efficiency(void) {
    apply("2", "/proc/ppm/mode");
    mtk_fpsgo(3);
    apply("0", "/proc/perfmgr/syslimiter/syslimiter_force_disable");
    apply("0", "/sys/devices/platform/boot_dramboost/dramboost/dramboost");
    apply("1", "/sys/devices/system/cpu/eas/enable");
    apply("2", "/proc/cpufreq/cpufreq_cci_mode");
    apply("1", "/proc/cpufreq/cpufreq_power_mode");
    if (file_exists("/proc/gpufreqv2/gpu_working_opp_table")) apply_ll(mtk_gpufreq_minfreq_index("/proc/gpufreqv2/gpu_working_opp_table"), "/proc/gpufreqv2/fix_target_opp_index");
    else if (file_exists("/proc/gpufreq/gpufreq_opp_dump")) apply_ll(mtk_gpufreq_min_freq("/proc/gpufreq/gpufreq_opp_dump"), "/proc/gpufreq/gpufreq_opp_freq");
}
