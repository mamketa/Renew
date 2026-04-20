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

/* Shared helper: apply a power policy to the first detected Mali platform device */
static void _mali_power_policy(const char *policy) {
    const char *mali = find_mali_platform_path();
    if (!mali) return;
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s/power_policy", mali);
    apply(policy, path);
}

/* Shared: apply devfreq action on all devfreq_mif entries */
static void _mif_devfreq(int mode) {
    DIR *dir = opendir("/sys/class/devfreq");
    if (!dir) return;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (!strstr(ent->d_name, "devfreq_mif")) continue;
        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "/sys/class/devfreq/%s", ent->d_name);
        if (mode == 1)       devfreq_max_perf(path);
        else if (mode == 2)  devfreq_mid_perf(path);
        else if (mode == 3)  devfreq_min_perf(path);
        else                 devfreq_unlock(path);
    }
    closedir(dir);
}

void exynos_esport(void) {
    /* Exynos GPU via /sys/kernel/gpu */
    const char *gpu = "/sys/kernel/gpu";
    if (node_exists(gpu)) {
        char avail[MAX_PATH_LEN];
        snprintf(avail, sizeof(avail), "%s/gpu_available_frequencies", gpu);
        long max_freq = get_max_freq(avail);

        char p[MAX_PATH_LEN];
        snprintf(p, sizeof(p), "%s/gpu_max_clock", gpu);
        apply_ll(max_freq, p);

        snprintf(p, sizeof(p), "%s/gpu_min_clock", gpu);
        if (LITE_MODE == 1) {
            long mid_freq = get_mid_freq(avail);
            apply_ll(mid_freq, p);
        } else {
            apply_ll(max_freq, p);
        }
    }

    /* Mali power policy: always_on for esport */
    _mali_power_policy("always_on");

    /* DRAM and bus frequency */
    if (DEVICE_MITIGATION == 0) {
        if (LITE_MODE == 1)
            _mif_devfreq(2);
        else
            _mif_devfreq(1);
    }
}

void exynos_balanced(void) {
    /* Exynos GPU: unlock */
    const char *gpu = "/sys/kernel/gpu";
    if (node_exists(gpu)) {
        char avail[MAX_PATH_LEN];
        snprintf(avail, sizeof(avail), "%s/gpu_available_frequencies", gpu);
        long max_freq = get_max_freq(avail);
        long min_freq = get_min_freq(avail);

        char p[MAX_PATH_LEN];
        snprintf(p, sizeof(p), "%s/gpu_max_clock", gpu);
        write_ll(max_freq, p);
        snprintf(p, sizeof(p), "%s/gpu_min_clock", gpu);
        write_ll(min_freq, p);
    }

    /* Mali: coarse demand for balanced */
    _mali_power_policy("coarse_demand");

    /* DRAM: unlock */
    if (DEVICE_MITIGATION == 0)
        _mif_devfreq(0);
}

void exynos_efficiency(void) {
    /* GPU: minimum frequency */
    const char *gpu = "/sys/kernel/gpu";
    if (node_exists(gpu)) {
        char avail[MAX_PATH_LEN];
        snprintf(avail, sizeof(avail), "%s/gpu_available_frequencies", gpu);
        long min_freq = get_min_freq(avail);

        char p[MAX_PATH_LEN];
        snprintf(p, sizeof(p), "%s/gpu_min_clock", gpu);
        apply_ll(min_freq, p);
        snprintf(p, sizeof(p), "%s/gpu_max_clock", gpu);
        apply_ll(min_freq, p);
    }

    /* Mali: adaptive for power saving */
    _mali_power_policy("adaptive");

    /* DRAM: minimum */
    if (DEVICE_MITIGATION == 0)
        _mif_devfreq(3);
}
