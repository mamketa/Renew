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
#include "velfox_devfreq.h"

static bool qcom_bus_name(const char *name, void *ctx) {
    (void)ctx;
    return name_contains(name, "cpu-lat") || name_contains(name, "cpu-bw") || name_contains(name, "llccbw") || name_contains(name, "bus_llcc") || name_contains(name, "bus_ddr") || name_contains(name, "memlat") || name_contains(name, "cpubw") || name_contains(name, "kgsl-ddr-qos");
}

static void qcom_bus_handler(const char *dir, const char *name, void *ctx) {
    int mode = *(int *)ctx;
    char path[MAX_PATH_LEN];
    if (!path_join(path, sizeof(path), dir, name)) return;
    if (mode == 1) LITE_MODE == 1 ? devfreq_mid_perf(path) : devfreq_max_perf(path);
    else if (mode == 2) devfreq_unlock(path);
    else devfreq_min_perf(path);
}

static void qcom_bus_dcvs(int mode) {
    const char *components[] = {"DDR", "LLCC", "L3"};
    for (size_t i = 0; i < sizeof(components) / sizeof(components[0]); ++i) {
        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/bus_dcvs/%s", components[i]);
        if (mode == 1) LITE_MODE == 1 ? qcom_cpudcvs_mid_perf(path) : qcom_cpudcvs_max_perf(path);
        else if (mode == 2) qcom_cpudcvs_unlock(path);
        else qcom_cpudcvs_min_perf(path);
    }
}

static void qcom_bw_hwmon(int mode) {
    const char *base = "/sys/class/devfreq/soc:qcom,cpu-bw/bw_hwmon";
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s/io_percent", base);
    apply(mode == 1 ? "80" : mode == 2 ? "60" : "40", path);
    snprintf(path, sizeof(path), "%s/sample_ms", base);
    apply(mode == 1 ? "4" : mode == 2 ? "8" : "16", path);
    snprintf(path, sizeof(path), "%s/hist_memory", base);
    apply(mode == 1 ? "20" : mode == 2 ? "10" : "5", path);
    snprintf(path, sizeof(path), "%s/decay_rate", base);
    apply(mode == 1 ? "90" : mode == 2 ? "80" : "60", path);
    snprintf(path, sizeof(path), "%s/bw_step", base);
    apply(mode == 1 ? "190" : mode == 2 ? "120" : "80", path);
}

static void adreno(int mode) {
    if (mode == 1) LITE_MODE == 0 ? devfreq_max_perf("/sys/class/kgsl/kgsl-3d0/devfreq") : devfreq_mid_perf("/sys/class/kgsl/kgsl-3d0/devfreq");
    else if (mode == 2) devfreq_unlock("/sys/class/kgsl/kgsl-3d0/devfreq");
    else devfreq_min_perf("/sys/class/kgsl/kgsl-3d0/devfreq");
    apply(mode == 1 ? "0" : "1", "/sys/class/kgsl/kgsl-3d0/throttling");
    apply(mode == 1 ? "0" : "1", "/sys/class/kgsl/kgsl-3d0/thermal_pwrlevel");
    apply(mode == 1 ? "1" : "0", "/sys/class/kgsl/kgsl-3d0/devfreq/adrenoboost");
    apply(mode == 1 ? "1" : "0", "/sys/class/kgsl/kgsl-3d0/force_bus_on");
    apply(mode == 1 ? "1" : "0", "/sys/class/kgsl/kgsl-3d0/force_clk_on");
    apply(mode == 1 ? "1" : "0", "/sys/class/kgsl/kgsl-3d0/force_no_nap");
    apply(mode == 1 ? "0" : "1", "/sys/class/kgsl/kgsl-3d0/bus_split");
    apply("0", "/sys/class/kgsl/kgsl-3d0/perfcounter");
}

static void snapdragon_mode(int mode) {
    if (DEVICE_MITIGATION == 0) {
        for_each_dir_entry("/sys/class/devfreq", qcom_bus_name, qcom_bus_handler, &(int){mode});
        qcom_bus_dcvs(mode);
        qcom_bw_hwmon(mode);
    }
    adreno(mode);
}

void snapdragon_esport(void) { snapdragon_mode(1); }
void snapdragon_balanced(void) { snapdragon_mode(2); }
void snapdragon_efficiency(void) { snapdragon_mode(3); }
