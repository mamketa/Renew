#include "velfox_soc.h"
#include "velfox_cpufreq.h"
#include "velfox_devfreq.h"

static void set_mtk_ppm_policy(int enabled) {
    if (!PPM_POLICY[0] || !file_exists("/proc/ppm/policy_status")) return;
    FILE *fp = fopen("/proc/ppm/policy_status", "r");
    if (!fp) return;
    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, PPM_POLICY)) {
            char policy_cmd[16];
            snprintf(policy_cmd, sizeof(policy_cmd), "%c %d", line[1], enabled);
            write_sysfs("/proc/ppm/policy_status", policy_cmd);
        }
    }
    fclose(fp);
}

static long mtk_gpufreq_first_freq(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    char line[MAX_LINE_LEN];
    long freq = 0;
    if (fgets(line, sizeof(line), fp)) {
        char *freq_str = strstr(line, "freq =");
        if (freq_str) freq = atol(freq_str + 6);
    }
    fclose(fp);
    return freq;
}

static long mtk_gpufreq_lowest_freq(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    char line[MAX_LINE_LEN];
    long min_freq = LONG_MAX;
    while (fgets(line, sizeof(line), fp)) {
        char *freq_str = strstr(line, "freq =");
        if (freq_str) {
            long freq = atol(freq_str + 6);
            if (freq > 0 && freq < min_freq) min_freq = freq;
        }
    }
    fclose(fp);
    return min_freq == LONG_MAX ? 0 : min_freq;
}

static void tune_mtk_gpu(int mode) {
    if (mode == 1) {
        if (LITE_MODE == 0) {
            if (file_exists("/proc/gpufreqv2/fix_target_opp_index")) {
                write_sysfs("/proc/gpufreqv2/fix_target_opp_index", "0");
            } else if (file_exists("/proc/gpufreq/gpufreq_opp_dump")) {
                long freq = mtk_gpufreq_first_freq("/proc/gpufreq/gpufreq_opp_dump");
                if (freq > 0) write_ll(freq, "/proc/gpufreq/gpufreq_opp_freq");
            }
        } else {
            write_sysfs("/proc/gpufreq/gpufreq_opp_freq", "0");
            write_sysfs("/proc/gpufreqv2/fix_target_opp_index", "-1");
            int mid_opp = 0;
            if (file_exists("/proc/gpufreqv2/gpu_working_opp_table")) mid_opp = mtk_gpufreq_midfreq_index("/proc/gpufreqv2/gpu_working_opp_table");
            else if (file_exists("/proc/gpufreq/gpufreq_opp_dump")) mid_opp = mtk_gpufreq_midfreq_index("/proc/gpufreq/gpufreq_opp_dump");
            write_ll(mid_opp, "/sys/kernel/ged/hal/custom_boost_gpu_freq");
        }
    } else if (mode == 2) {
        write_sysfs("/proc/gpufreq/gpufreq_opp_freq", "0");
        write_sysfs("/proc/gpufreqv2/fix_target_opp_index", "-1");
        write_sysfs("/sys/module/ged/parameters/gpu_dvfs_enable", "1");
        int min_opp = 0;
        if (file_exists("/proc/gpufreqv2/gpu_working_opp_table")) min_opp = mtk_gpufreq_minfreq_index("/proc/gpufreqv2/gpu_working_opp_table");
        else if (file_exists("/proc/gpufreq/gpufreq_opp_dump")) min_opp = mtk_gpufreq_minfreq_index("/proc/gpufreq/gpufreq_opp_dump");
        write_ll(min_opp, "/sys/kernel/ged/hal/custom_boost_gpu_freq");
    } else {
        if (file_exists("/proc/gpufreqv2/gpu_working_opp_table")) {
            int min_opp = mtk_gpufreq_minfreq_index("/proc/gpufreqv2/gpu_working_opp_table");
            write_ll(min_opp, "/proc/gpufreqv2/fix_target_opp_index");
        } else if (file_exists("/proc/gpufreq/gpufreq_opp_dump")) {
            long min_freq = mtk_gpufreq_lowest_freq("/proc/gpufreq/gpufreq_opp_dump");
            if (min_freq > 0) write_ll(min_freq, "/proc/gpufreq/gpufreq_opp_freq");
        }
    }
}

static int mtk_dvfsrc_cb(const char *entry_path, const char *entry_name, void *userdata) {
    int mode = *((int *)userdata);
    if (!strstr(entry_name, "dvfsrc")) return 0;
    if (mode == 1) return LITE_MODE == 1 ? devfreq_mid_perf(entry_path) : devfreq_max_perf(entry_path);
    if (mode == 2) return devfreq_unlock(entry_path);
    return devfreq_mid_perf(entry_path);
}

static void tune_mtk_dram(int mode) {
    if (mode == 1 && LITE_MODE == 0) {
        write_sysfs("/sys/devices/platform/10012000.dvfsrc/helio-dvfsrc/dvfsrc_req_ddr_opp", "0");
        write_sysfs("/sys/kernel/helio-dvfsrc/dvfsrc_force_vcore_dvfs_opp", "0");
    } else {
        write_sysfs("/sys/devices/platform/10012000.dvfsrc/helio-dvfsrc/dvfsrc_req_ddr_opp", "-1");
        write_sysfs("/sys/kernel/helio-dvfsrc/dvfsrc_force_vcore_dvfs_opp", "-1");
    }
    iterate_sysfs_nodes("/sys/class/devfreq", NULL, mtk_dvfsrc_cb, &mode);
}

static void set_mtk_gpu_limiter(int disabled) {
    write_sysfs("/proc/gpufreq/gpufreq_power_limited", disabled ? "ignore_batt_oc 1" : "ignore_batt_oc 0");
    write_sysfs("/proc/gpufreq/gpufreq_power_limited", disabled ? "ignore_batt_percent 1" : "ignore_batt_percent 0");
    write_sysfs("/proc/gpufreq/gpufreq_power_limited", disabled ? "ignore_low_batt 1" : "ignore_low_batt 0");
    write_sysfs("/proc/gpufreq/gpufreq_power_limited", disabled ? "ignore_thermal_protect 1" : "ignore_thermal_protect 0");
    write_sysfs("/proc/gpufreq/gpufreq_power_limited", disabled ? "ignore_pbm_limited 1" : "ignore_pbm_limited 0");
}

void mediatek_esport() {
    set_mtk_ppm_policy(0);
    write_sysfs("/sys/kernel/fpsgo/fstb/adopt_low_fps", "0");
    write_sysfs("/sys/pnpmgr/fpsgo_boost/boost_enable", "1");
    write_sysfs("/sys/kernel/fpsgo/fstb/gpu_slowdown_check", "1");
    write_sysfs("/proc/cpufreq/cpufreq_cci_mode", "1");
    write_sysfs("/proc/cpufreq/cpufreq_power_mode", "3");
    write_sysfs("/proc/perfmgr/syslimiter/syslimiter_force_disable", "1");
    write_sysfs("/sys/devices/platform/boot_dramboost/dramboost/dramboost", "1");
    write_sysfs("/sys/devices/system/cpu/eas/enable", "0");
    tune_mtk_gpu(1);
    set_mtk_gpu_limiter(1);
    write_sysfs("/proc/mtk_batoc_throttling/battery_oc_protect_stop", "stop 1");
    tune_mtk_dram(1);
    write_sysfs("/sys/kernel/eara_thermal/enable", "0");
}

void mediatek_balanced() {
    set_mtk_ppm_policy(1);
    write_sysfs("/sys/kernel/fpsgo/common/force_onoff", "1");
    write_sysfs("/sys/kernel/fpsgo/fstb/adopt_low_fps", "1");
    write_sysfs("/sys/pnpmgr/fpsgo_boost/boost_enable", "0");
    write_sysfs("/sys/kernel/fpsgo/fstb/gpu_slowdown_check", "0");
    write_sysfs("/sys/kernel/fpsgo/fstb/fstb_self_ctrl_fps_enable", "1");
    write_sysfs("/sys/kernel/fpsgo/fbt/enable_switch_down_throttle", "0");
    write_sysfs("/sys/module/mtk_fpsgo/parameters/perfmgr_enable", "1");
    write_sysfs("/proc/cpufreq/cpufreq_cci_mode", "0");
    write_sysfs("/proc/cpufreq/cpufreq_power_mode", "2");
    write_sysfs("/proc/perfmgr/syslimiter/syslimiter_force_disable", "0");
    write_sysfs("/sys/devices/platform/boot_dramboost/dramboost/dramboost", "0");
    write_sysfs("/sys/devices/system/cpu/eas/enable", "2");
    write_sysfs("/sys/module/ged/parameters/is_GED_KPI_enabled", "0");
    tune_mtk_gpu(2);
    set_mtk_gpu_limiter(0);
    write_sysfs("/proc/mtk_batoc_throttling/battery_oc_protect_stop", "stop 0");
    tune_mtk_dram(2);
    write_sysfs("/sys/kernel/eara_thermal/enable", "1");
}

void mediatek_efficiency() {
    set_mtk_ppm_policy(1);
    write_sysfs("/sys/kernel/fpsgo/fstb/adopt_low_fps", "1");
    write_sysfs("/sys/pnpmgr/fpsgo_boost/boost_enable", "0");
    write_sysfs("/sys/kernel/fpsgo/fstb/gpu_slowdown_check", "0");
    write_sysfs("/sys/kernel/fpsgo/fbt/enable_switch_down_throttle", "1");
    write_sysfs("/sys/module/mtk_fpsgo/parameters/perfmgr_enable", "1");
    write_sysfs("/proc/perfmgr/syslimiter/syslimiter_force_disable", "0");
    write_sysfs("/sys/devices/platform/boot_dramboost/dramboost/dramboost", "0");
    write_sysfs("/sys/devices/system/cpu/eas/enable", "1");
    write_sysfs("/proc/cpufreq/cpufreq_cci_mode", "1");
    write_sysfs("/proc/cpufreq/cpufreq_power_mode", "1");
    tune_mtk_gpu(3);
    set_mtk_gpu_limiter(0);
    tune_mtk_dram(3);
}
