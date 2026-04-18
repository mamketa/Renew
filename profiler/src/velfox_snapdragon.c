#include "velfox_soc.h"
#include "velfox_cpufreq.h"
#include "velfox_devfreq.h"

static int qcom_bus_match(const char *name) {
    const char *tokens[] = {"cpu-lat", "cpu_lat", "cpu-bw", "cpubw", "llcc", "l3", "bw-hwmon", "bw_hwmon", "bus_llcc", "bus_ddr", "memlat", "kgsl-ddr-qos"};
    for (size_t i = 0; i < sizeof(tokens) / sizeof(tokens[0]); i++) {
        if (strstr(name, tokens[i])) return 1;
    }
    return 0;
}

static int qcom_devfreq_cb(const char *entry_path, const char *entry_name, void *userdata) {
    int mode = *((int *)userdata);
    if (!qcom_bus_match(entry_name)) return 0;
    if (mode == 1) return LITE_MODE == 1 ? devfreq_mid_perf(entry_path) : devfreq_max_perf(entry_path);
    if (mode == 2) return devfreq_unlock(entry_path);
    return devfreq_mid_perf(entry_path);
}

static void tune_qcom_bus_devfreq(int mode) {
    if (DEVICE_MITIGATION != 0) return;
    iterate_sysfs_nodes("/sys/class/devfreq", NULL, qcom_devfreq_cb, &mode);
}

static void tune_qcom_bus_dcvs(int mode) {
    if (DEVICE_MITIGATION != 0) return;
    const char *components[] = {"DDR", "LLCC", "L3"};
    for (size_t i = 0; i < sizeof(components) / sizeof(components[0]); i++) {
        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/bus_dcvs/%s", components[i]);
        if (mode == 1) {
            if (LITE_MODE == 1) qcom_cpudcvs_mid_perf(path);
            else qcom_cpudcvs_max_perf(path);
        } else if (mode == 2) {
            qcom_cpudcvs_unlock(path);
        } else {
            qcom_cpudcvs_mid_perf(path);
        }
    }
}

static void tune_adreno_gpu(int mode) {
    const char *gpu_path = "/sys/class/kgsl/kgsl-3d0/devfreq";
    if (mode == 1) {
        if (LITE_MODE == 0) devfreq_max_perf(gpu_path);
        else devfreq_mid_perf(gpu_path);
        write_sysfs("/sys/class/kgsl/kgsl-3d0/throttling", "0");
        write_sysfs("/sys/class/kgsl/kgsl-3d0/thermal_pwrlevel", "0");
        write_sysfs("/sys/class/kgsl/kgsl-3d0/devfreq/adrenoboost", "1");
        write_sysfs("/sys/class/kgsl/kgsl-3d0/force_clk_on", "1");
        write_sysfs("/sys/class/kgsl/kgsl-3d0/force_no_nap", "1");
        write_sysfs("/sys/class/kgsl/kgsl-3d0/bus_split", "0");
    } else if (mode == 2) {
        devfreq_unlock(gpu_path);
        write_sysfs("/sys/class/kgsl/kgsl-3d0/throttling", "1");
        write_sysfs("/sys/class/kgsl/kgsl-3d0/thermal_pwrlevel", "1");
        write_sysfs("/sys/class/kgsl/kgsl-3d0/devfreq/adrenoboost", "0");
        write_sysfs("/sys/class/kgsl/kgsl-3d0/force_clk_on", "0");
        write_sysfs("/sys/class/kgsl/kgsl-3d0/force_no_nap", "0");
        write_sysfs("/sys/class/kgsl/kgsl-3d0/bus_split", "1");
    } else {
        devfreq_mid_perf(gpu_path);
        write_sysfs("/sys/class/kgsl/kgsl-3d0/throttling", "1");
        write_sysfs("/sys/class/kgsl/kgsl-3d0/thermal_pwrlevel", "1");
        write_sysfs("/sys/class/kgsl/kgsl-3d0/devfreq/adrenoboost", "0");
        write_sysfs("/sys/class/kgsl/kgsl-3d0/force_clk_on", "0");
        write_sysfs("/sys/class/kgsl/kgsl-3d0/force_no_nap", "0");
        write_sysfs("/sys/class/kgsl/kgsl-3d0/bus_split", "1");
    }
    write_sysfs("/sys/class/kgsl/kgsl-3d0/perfcounter", "0");
}

void snapdragon_esport() {
    tune_qcom_bus_devfreq(1);
    tune_qcom_bus_dcvs(1);
    tune_adreno_gpu(1);
}

void snapdragon_balanced() {
    tune_qcom_bus_devfreq(2);
    tune_qcom_bus_dcvs(2);
    tune_adreno_gpu(2);
}

void snapdragon_efficiency() {
    tune_qcom_bus_devfreq(3);
    tune_qcom_bus_dcvs(3);
    tune_adreno_gpu(3);
}
