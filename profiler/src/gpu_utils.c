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

#include "gpu_utils.h"
#include "cpu_utils.h"
#include "utility_utils.h"

/*
 * find_gpu_path:
 * Probes known GPU devfreq locations in priority order:
 *   1. kgsl-3d0/devfreq      (Adreno/Snapdragon)
 *   2. /sys/class/devfreq/mali*  (Mali devfreq-backed)
 *   3. /sys/class/devfreq/<name>.gpu  (Generic devfreq GPU, Unisoc)
 * Returns pointer to static buffer or NULL.
 */
const char *find_gpu_path(void) {
    static char _gpu_path[MAX_PATH_LEN] = {0};
    if (_gpu_path[0] != '\0') return _gpu_path;

    /* 1. Adreno KGSL */
    static const char *kgsl_paths[] = {
        "/sys/class/kgsl/kgsl-3d0/devfreq",
        "/sys/class/kgsl/kgsl-3d0",
    };
    for (int i = 0; i < 2; i++) {
        if (node_exists(kgsl_paths[i])) {
            snprintf(_gpu_path, sizeof(_gpu_path), "%s", kgsl_paths[i]);
            return _gpu_path;
        }
    }

    /* 2. Mali / generic devfreq GPU — scan /sys/class/devfreq */
    DIR *dir = opendir("/sys/class/devfreq");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            const char *n = ent->d_name;
            if (n[0] == '.') continue;
            if (strstr(n, "mali") || strstr(n, ".gpu") ||
                strstr(n, "gpu") || strstr(n, "kgsl")) {
                snprintf(_gpu_path, sizeof(_gpu_path),
                         "/sys/class/devfreq/%s", n);
                /* Verify available_frequencies exists (confirms it's a real GPU devfreq) */
                char test[MAX_PATH_LEN];
                snprintf(test, sizeof(test), "%s/available_frequencies", _gpu_path);
                if (node_exists(test)) {
                    closedir(dir);
                    return _gpu_path;
                }
            }
        }
        closedir(dir);
    }

    _gpu_path[0] = '\0';
    return NULL;
}

/*
 * find_mali_platform_path:
 * Scans /sys/devices/platform for *.mali entries.
 * Returns pointer to static buffer or NULL.
 */
const char *find_mali_platform_path(void) {
    static char _mali_path[MAX_PATH_LEN] = {0};
    if (_mali_path[0] != '\0') return _mali_path;

    DIR *dir = opendir("/sys/devices/platform");
    if (!dir) return NULL;

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strstr(ent->d_name, ".mali")) {
            snprintf(_mali_path, sizeof(_mali_path),
                     "/sys/devices/platform/%s", ent->d_name);
            closedir(dir);
            return _mali_path;
        }
    }
    closedir(dir);
    _mali_path[0] = '\0';
    return NULL;
}

int gpu_max_perf(const char *gpu_devfreq_path) {
    if (!gpu_devfreq_path || !node_exists(gpu_devfreq_path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", gpu_devfreq_path);
    if (!node_exists(freq_path)) return 0;
    long max_freq = get_max_freq(freq_path);
    if (max_freq <= 0) return 0;
    char p[MAX_PATH_LEN];
    snprintf(p, sizeof(p), "%s/max_freq", gpu_devfreq_path);
    apply_ll(max_freq, p);
    snprintf(p, sizeof(p), "%s/min_freq", gpu_devfreq_path);
    apply_ll(max_freq, p);
    return 1;
}

int gpu_mid_perf(const char *gpu_devfreq_path) {
    if (!gpu_devfreq_path || !node_exists(gpu_devfreq_path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", gpu_devfreq_path);
    if (!node_exists(freq_path)) return 0;
    long max_freq = get_max_freq(freq_path);
    long mid_freq = get_mid_freq(freq_path);
    if (max_freq <= 0 || mid_freq <= 0) return 0;
    char p[MAX_PATH_LEN];
    snprintf(p, sizeof(p), "%s/max_freq", gpu_devfreq_path);
    apply_ll(max_freq, p);
    snprintf(p, sizeof(p), "%s/min_freq", gpu_devfreq_path);
    apply_ll(mid_freq, p);
    return 1;
}

int gpu_min_perf(const char *gpu_devfreq_path) {
    if (!gpu_devfreq_path || !node_exists(gpu_devfreq_path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", gpu_devfreq_path);
    if (!node_exists(freq_path)) return 0;
    long min_freq = get_min_freq(freq_path);
    if (min_freq <= 0) return 0;
    char p[MAX_PATH_LEN];
    snprintf(p, sizeof(p), "%s/min_freq", gpu_devfreq_path);
    apply_ll(min_freq, p);
    snprintf(p, sizeof(p), "%s/max_freq", gpu_devfreq_path);
    apply_ll(min_freq, p);
    return 1;
}

int gpu_unlock(const char *gpu_devfreq_path) {
    if (!gpu_devfreq_path || !node_exists(gpu_devfreq_path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", gpu_devfreq_path);
    if (!node_exists(freq_path)) return 0;
    long max_freq = get_max_freq(freq_path);
    long min_freq = get_min_freq(freq_path);
    if (max_freq <= 0 || min_freq <= 0) return 0;
    char p[MAX_PATH_LEN];
    snprintf(p, sizeof(p), "%s/max_freq", gpu_devfreq_path);
    write_ll(max_freq, p);
    snprintf(p, sizeof(p), "%s/min_freq", gpu_devfreq_path);
    write_ll(min_freq, p);
    return 1;
}
