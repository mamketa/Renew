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

#include "soc_utils.h"
#include "cpu_utils.h"
#include "devfreq_utils.h"
#include "utility_utils.h"
#include "velfox_common.h"

/* Apply PPM policy lines matching PPM_POLICY string with enable flag (0=disable, 1=enable) */
static void _mtk_ppm_policy(int enable) {
    if (!node_exists("/proc/ppm/policy_status")) return;
    FILE *fp = fopen("/proc/ppm/policy_status", "r");
    if (!fp) return;
    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, PPM_POLICY)) {
            char policy_cmd[16];
            snprintf(policy_cmd, sizeof(policy_cmd), "%c %d", line[1], enable);
            apply(policy_cmd, "/proc/ppm/policy_status");
        }
    }
    fclose(fp);
}

void mediatek_esport(void) {
    /* PPM: disable conflicting policies */
    _mtk_ppm_policy(0);

    /* FPSGO tweaks */
    apply("0", "/sys/kernel/fpsgo/fstb/adopt_low_fps");
    apply("1", "/sys/pnpmgr/fpsgo_boost/boost_enable");
    apply("1", "/sys/kernel/fpsgo/fstb/gpu_slowdown_check");

    /* MTK Power and CCI mode */
    apply("1", "/proc/cpufreq/cpufreq_cci_mode");
    apply("3", "/proc/cpufreq/cpufreq_power_mode");

    /* Perf limiter ON */
    apply("1", "/proc/perfmgr/syslimiter/syslimiter_force_disable");

    /* DDR Boost mode */
    apply("1", "/sys/devices/platform/boot_dramboost/dramboost/dramboost");

    /* EAS/HMP switch */
    apply("0", "/sys/devices/system/cpu/eas/enable");

    /* GPU Frequency */
    if (LITE_MODE == 0) {
        if (node_exists("/proc/gpufreqv2")) {
            apply("0", "/proc/gpufreqv2/fix_target_opp_index");
        } else if (node_exists("/proc/gpufreq/gpufreq_opp_dump")) {
            FILE *fp = fopen("/proc/gpufreq/gpufreq_opp_dump", "r");
            if (fp) {
                char line[MAX_LINE_LEN];
                if (fgets(line, sizeof(line), fp)) {
                    char *freq_str = strstr(line, "freq =");
                    if (freq_str)
                        apply_ll(atol(freq_str + 6), "/proc/gpufreq/gpufreq_opp_freq");
                }
                fclose(fp);
            }
        }
    } else {
        apply("0",  "/proc/gpufreq/gpufreq_opp_freq");
        apply("-1", "/proc/gpufreqv2/fix_target_opp_index");

        int mid_opp = 0;
        if (node_exists("/proc/gpufreqv2/gpu_working_opp_table"))
            mid_opp = mtk_gpufreq_midfreq_index("/proc/gpufreqv2/gpu_working_opp_table");
        else if (node_exists("/proc/gpufreq/gpufreq_opp_dump"))
            mid_opp = mtk_gpufreq_midfreq_index("/proc/gpufreq/gpufreq_opp_dump");

        apply_ll(mid_opp, "/sys/kernel/ged/hal/custom_boost_gpu_freq");
    }

    /* Disable GPU power limiter */
    apply("ignore_batt_oc 1",          "/proc/gpufreq/gpufreq_power_limited");
    apply("ignore_batt_percent 1",     "/proc/gpufreq/gpufreq_power_limited");
    apply("ignore_low_batt 1",         "/proc/gpufreq/gpufreq_power_limited");
    apply("ignore_thermal_protect 1",  "/proc/gpufreq/gpufreq_power_limited");
    apply("ignore_pbm_limited 1",      "/proc/gpufreq/gpufreq_power_limited");

    /* Disable battery current limiter */
    apply("stop 1", "/proc/mtk_batoc_throttling/battery_oc_protect_stop");

    /* DRAM Frequency */
    if (LITE_MODE == 0) {
        apply("0", "/sys/devices/platform/10012000.dvfsrc/helio-dvfsrc/dvfsrc_req_ddr_opp");
        apply("0", "/sys/kernel/helio-dvfsrc/dvfsrc_force_vcore_dvfs_opp");
        devfreq_max_perf("/sys/class/devfreq/mtk-dvfsrc-devfreq");
    } else {
        apply("-1", "/sys/devices/platform/10012000.dvfsrc/helio-dvfsrc/dvfsrc_req_ddr_opp");
        apply("-1", "/sys/kernel/helio-dvfsrc/dvfsrc_force_vcore_dvfs_opp");
        devfreq_mid_perf("/sys/class/devfreq/mtk-dvfsrc-devfreq");
    }

    /* Eara Thermal */
    apply("0", "/sys/kernel/eara_thermal/enable");
}

void mediatek_balanced(void) {
    /* PPM: re-enable policies */
    _mtk_ppm_policy(1);

    /* FPSGO tweaks */
    apply("1", "/sys/kernel/fpsgo/common/force_onoff");
    apply("1", "/sys/kernel/fpsgo/fstb/adopt_low_fps");
    apply("0", "/sys/pnpmgr/fpsgo_boost/boost_enable");
    apply("0", "/sys/kernel/fpsgo/fstb/gpu_slowdown_check");
    apply("1", "/sys/kernel/fpsgo/fstb/fstb_self_ctrl_fps_enable");
    apply("0", "/sys/kernel/fpsgo/fbt/enable_switch_down_throttle");
    apply("1", "/sys/module/mtk_fpsgo/parameters/perfmgr_enable");

    /* MTK Power and CCI mode */
    apply("0", "/proc/cpufreq/cpufreq_cci_mode");
    apply("2", "/proc/cpufreq/cpufreq_power_mode");

    /* Perf limiter */
    apply("0", "/proc/perfmgr/syslimiter/syslimiter_force_disable");

    /* DDR Boost mode */
    apply("0", "/sys/devices/platform/boot_dramboost/dramboost/dramboost");

    /* EAS/HMP */
    apply("2", "/sys/devices/system/cpu/eas/enable");

    /* Disable GED KPI */
    apply("0", "/sys/module/ged/parameters/is_GED_KPI_enabled");

    /* GPU Frequency: unlock */
    write_file("0",  "/proc/gpufreq/gpufreq_opp_freq");
    write_file("-1", "/proc/gpufreqv2/fix_target_opp_index");

    /* Enable GPU dynamic scaling */
    apply("1", "/sys/module/ged/parameters/gpu_dvfs_enable");

    /* Reset min freq via GED */
    int min_opp = 0;
    if (node_exists("/proc/gpufreqv2/gpu_working_opp_table"))
        min_opp = mtk_gpufreq_minfreq_index("/proc/gpufreqv2/gpu_working_opp_table");
    else if (node_exists("/proc/gpufreq/gpufreq_opp_dump"))
        min_opp = mtk_gpufreq_minfreq_index("/proc/gpufreq/gpufreq_opp_dump");

    apply_ll(min_opp, "/sys/kernel/ged/hal/custom_boost_gpu_freq");

    /* GPU power limiter: restore */
    apply("ignore_batt_oc 0",          "/proc/gpufreq/gpufreq_power_limited");
    apply("ignore_batt_percent 0",     "/proc/gpufreq/gpufreq_power_limited");
    apply("ignore_low_batt 0",         "/proc/gpufreq/gpufreq_power_limited");
    apply("ignore_thermal_protect 0",  "/proc/gpufreq/gpufreq_power_limited");
    apply("ignore_pbm_limited 0",      "/proc/gpufreq/gpufreq_power_limited");

    /* Battery current limiter: restore */
    apply("stop 0", "/proc/mtk_batoc_throttling/battery_oc_protect_stop");

    /* DRAM Frequency: unlock */
    write_file("-1", "/sys/devices/platform/10012000.dvfsrc/helio-dvfsrc/dvfsrc_req_ddr_opp");
    write_file("-1", "/sys/kernel/helio-dvfsrc/dvfsrc_force_vcore_dvfs_opp");
    devfreq_unlock("/sys/class/devfreq/mtk-dvfsrc-devfreq");

    /* Eara Thermal */
    apply("1", "/sys/kernel/eara_thermal/enable");
}

void mediatek_efficiency(void) {
    /* FPSGO tweaks */
    apply("1", "/sys/kernel/fpsgo/fstb/adopt_low_fps");
    apply("0", "/sys/pnpmgr/fpsgo_boost/boost_enable");
    apply("0", "/sys/kernel/fpsgo/fstb/gpu_slowdown_check");
    apply("1", "/sys/kernel/fpsgo/fbt/enable_switch_down_throttle");
    apply("1", "/sys/module/mtk_fpsgo/parameters/perfmgr_enable");

    /* Perf limiter */
    apply("0", "/proc/perfmgr/syslimiter/syslimiter_force_disable");

    /* DDR Boost mode */
    apply("0", "/sys/devices/platform/boot_dramboost/dramboost/dramboost");

    /* EAS/HMP */
    apply("1", "/sys/devices/system/cpu/eas/enable");

    /* GPU Frequency: minimum */
    write_file("0",  "/proc/gpufreq/gpufreq_opp_freq");
    write_file("-1", "/proc/gpufreqv2/fix_target_opp_index");

    /* Enable GPU dynamic scaling */
    apply("1", "/sys/module/ged/parameters/gpu_dvfs_enable");

    /* Set min freq via GED (lowest OPP) */
    int min_opp = 0;
    if (node_exists("/proc/gpufreqv2/gpu_working_opp_table"))
        min_opp = mtk_gpufreq_minfreq_index("/proc/gpufreqv2/gpu_working_opp_table");
    else if (node_exists("/proc/gpufreq/gpufreq_opp_dump"))
        min_opp = mtk_gpufreq_minfreq_index("/proc/gpufreq/gpufreq_opp_dump");

    apply_ll(min_opp, "/sys/kernel/ged/hal/custom_boost_gpu_freq");

    /* GPU power limiter: restore for efficiency */
    apply("ignore_batt_oc 0",          "/proc/gpufreq/gpufreq_power_limited");
    apply("ignore_batt_percent 0",     "/proc/gpufreq/gpufreq_power_limited");
    apply("ignore_low_batt 0",         "/proc/gpufreq/gpufreq_power_limited");
    apply("ignore_thermal_protect 0",  "/proc/gpufreq/gpufreq_power_limited");
    apply("ignore_pbm_limited 0",      "/proc/gpufreq/gpufreq_power_limited");

    /* Battery current limiter: restore */
    apply("stop 0", "/proc/mtk_batoc_throttling/battery_oc_protect_stop");

    /* DRAM Frequency: minimum */
    int max_ddr_opp = mtk_gpufreq_minfreq_index("/sys/class/devfreq/mtk-dvfsrc-devfreq/available_frequencies");
    apply_ll(max_ddr_opp, "/sys/devices/platform/10012000.dvfsrc/helio-dvfsrc/dvfsrc_req_ddr_opp");
    apply_ll(max_ddr_opp, "/sys/kernel/helio-dvfsrc/dvfsrc_force_vcore_dvfs_opp");
    devfreq_min_perf("/sys/class/devfreq/mtk-dvfsrc-devfreq");

    /* Eara Thermal */
    apply("1", "/sys/kernel/eara_thermal/enable");
}
