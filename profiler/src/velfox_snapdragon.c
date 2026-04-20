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
#include "gpu_utils.h"
#include "utility_utils.h"
#include "velfox_common.h"

/* Qualcomm CPU bus/DRAM devfreq node patterns */
static const char *const _qcom_bus_patterns[] = {
    "cpu-lat", "cpu-bw", "llccbw", "bus_llcc",
    "bus_ddr", "memlat", "cpubw", "kgsl-ddr-qos"
};
#define QCOM_BUS_PATTERN_COUNT 8

/* Qualcomm bus_dcvs components */
static const char *const _bus_dcvs_comps[] = {"DDR", "LLCC", "L3"};
#define BUS_DCVS_COUNT 3

static int _is_qcom_bus_dev(const char *name) {
    for (int i = 0; i < QCOM_BUS_PATTERN_COUNT; i++) {
        if (strstr(name, _qcom_bus_patterns[i])) return 1;
    }
    return 0;
}

static void _adreno_common(int throttle, int adrenoboost, int force_clk, int no_nap, int bus_split) {
    char val[4];
    snprintf(val, sizeof(val), "%d", throttle);
    apply(val, "/sys/class/kgsl/kgsl-3d0/throttling");
    apply(val, "/sys/class/kgsl/kgsl-3d0/thermal_pwrlevel");

    snprintf(val, sizeof(val), "%d", adrenoboost);
    apply(val, "/sys/class/kgsl/kgsl-3d0/devfreq/adrenoboost");

    snprintf(val, sizeof(val), "%d", force_clk);
    apply(val, "/sys/class/kgsl/kgsl-3d0/force_clk_on");

    snprintf(val, sizeof(val), "%d", no_nap);
    apply(val, "/sys/class/kgsl/kgsl-3d0/force_no_nap");

    snprintf(val, sizeof(val), "%d", bus_split);
    apply(val, "/sys/class/kgsl/kgsl-3d0/bus_split");

    /* Disable GPU perf counters in all modes */
    apply("0", "/sys/class/kgsl/kgsl-3d0/perfcounter");
}

void snapdragon_esport(void) {
    /* Qualcomm CPU bus and DRAM frequencies */
    if (DEVICE_MITIGATION == 0) {
        DIR *dir = opendir("/sys/class/devfreq");
        if (dir) {
            struct dirent *ent;
            while ((ent = readdir(dir)) != NULL) {
                if (!_is_qcom_bus_dev(ent->d_name)) continue;
                char path[MAX_PATH_LEN];
                snprintf(path, sizeof(path), "/sys/class/devfreq/%s", ent->d_name);
                if (LITE_MODE == 1)
                    devfreq_mid_perf(path);
                else
                    devfreq_max_perf(path);
            }
            closedir(dir);
        }

        /* bus_dcvs (DDR, LLCC, L3) */
        for (int i = 0; i < BUS_DCVS_COUNT; i++) {
            char path[MAX_PATH_LEN];
            snprintf(path, sizeof(path),
                     "/sys/devices/system/cpu/bus_dcvs/%s", _bus_dcvs_comps[i]);
            if (LITE_MODE == 1)
                qcom_cpudcvs_mid_perf(path);
            else
                qcom_cpudcvs_max_perf(path);
        }
    }

    /* KGSL GPU frequency */
    if (LITE_MODE == 1)
        devfreq_mid_perf("/sys/class/kgsl/kgsl-3d0/devfreq");
    else
        devfreq_max_perf("/sys/class/kgsl/kgsl-3d0/devfreq");

    /* Adreno: disable throttle, enable boost, keep clocks on */
    _adreno_common(0, 1, 1, 1, 0);
}

void snapdragon_balanced(void) {
    /* Unlock CPU bus devfreq */
    if (DEVICE_MITIGATION == 0) {
        DIR *dir = opendir("/sys/class/devfreq");
        if (dir) {
            struct dirent *ent;
            while ((ent = readdir(dir)) != NULL) {
                if (!_is_qcom_bus_dev(ent->d_name)) continue;
                char path[MAX_PATH_LEN];
                snprintf(path, sizeof(path), "/sys/class/devfreq/%s", ent->d_name);
                devfreq_unlock(path);
            }
            closedir(dir);
        }

        for (int i = 0; i < BUS_DCVS_COUNT; i++) {
            char path[MAX_PATH_LEN];
            snprintf(path, sizeof(path),
                     "/sys/devices/system/cpu/bus_dcvs/%s", _bus_dcvs_comps[i]);
            qcom_cpudcvs_unlock(path);
        }
    }

    /* GPU: unlock */
    devfreq_unlock("/sys/class/kgsl/kgsl-3d0/devfreq");

    /* Adreno: restore throttle, disable boost, allow nap */
    _adreno_common(1, 0, 0, 0, 1);
}

void snapdragon_efficiency(void) {
    /* GPU: minimum frequency */
    devfreq_min_perf("/sys/class/kgsl/kgsl-3d0/devfreq");

    /* Adreno: throttle on, no boost, allow idle */
    _adreno_common(1, 0, 0, 0, 1);

    /* Bus devfreq: evenly reduce to minimum for efficiency */
    if (DEVICE_MITIGATION == 0) {
        DIR *dir = opendir("/sys/class/devfreq");
        if (dir) {
            struct dirent *ent;
            while ((ent = readdir(dir)) != NULL) {
                if (!_is_qcom_bus_dev(ent->d_name)) continue;
                char path[MAX_PATH_LEN];
                snprintf(path, sizeof(path), "/sys/class/devfreq/%s", ent->d_name);
                devfreq_min_perf(path);
            }
            closedir(dir);
        }

        for (int i = 0; i < BUS_DCVS_COUNT; i++) {
            char path[MAX_PATH_LEN];
            snprintf(path, sizeof(path),
                     "/sys/devices/system/cpu/bus_dcvs/%s", _bus_dcvs_comps[i]);
            qcom_cpudcvs_min_perf(path);
        }
    }
}
