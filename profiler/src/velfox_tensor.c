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

/* Shared: apply devfreq action on all devfreq_mif entries (Tensor uses same MIF naming as Exynos) */
static void _tensor_mif_devfreq(int mode) {
    DIR *dir = opendir("/sys/class/devfreq");
    if (!dir) return;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (!strstr(ent->d_name, "devfreq_mif")) continue;
        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "/sys/class/devfreq/%s", ent->d_name);
        if (mode == 1)      devfreq_max_perf(path);
        else if (mode == 2) devfreq_mid_perf(path);
        else if (mode == 3) devfreq_min_perf(path);
        else                devfreq_unlock(path);
    }
    closedir(dir);
}

/*
 * Tensor GPU uses platform Mali path with scaling_min/max_freq.
 * Reuses find_mali_platform_path() from gpu_utils.
 */
static void _tensor_gpu_set(int mode) {
    const char *mali = find_mali_platform_path();
    if (!mali) return;

    char avail[MAX_PATH_LEN];
    snprintf(avail, sizeof(avail), "%s/available_frequencies", mali);
    if (!node_exists(avail)) return;

    long max_freq = get_max_freq(avail);
    char max_p[MAX_PATH_LEN], min_p[MAX_PATH_LEN];
    snprintf(max_p, sizeof(max_p), "%s/scaling_max_freq", mali);
    snprintf(min_p, sizeof(min_p), "%s/scaling_min_freq", mali);

    if (mode == 1) {
        /* Esport: max/max or max/mid in lite */
        apply_ll(max_freq, max_p);
        if (LITE_MODE == 1) {
            long mid = get_mid_freq(avail);
            apply_ll(mid, min_p);
        } else {
            apply_ll(max_freq, min_p);
        }
    } else if (mode == 2) {
        /* Balanced: unlock natural range */
        long min_freq = get_min_freq(avail);
        write_ll(max_freq, max_p);
        write_ll(min_freq, min_p);
    } else {
        /* Efficiency: lock to minimum */
        long min_freq = get_min_freq(avail);
        apply_ll(min_freq, min_p);
        apply_ll(min_freq, max_p);
    }
}

void tensor_esport(void) {
    _tensor_gpu_set(1);

    if (DEVICE_MITIGATION == 0) {
        if (LITE_MODE == 1)
            _tensor_mif_devfreq(2);
        else
            _tensor_mif_devfreq(1);
    }
}

void tensor_balanced(void) {
    _tensor_gpu_set(2);

    if (DEVICE_MITIGATION == 0)
        _tensor_mif_devfreq(0);
}

void tensor_efficiency(void) {
    _tensor_gpu_set(3);
}
