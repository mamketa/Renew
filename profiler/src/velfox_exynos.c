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

static void exynos_kernel_gpu(int mode) {
    const char *gpu = "/sys/kernel/gpu";
    if (!file_exists(gpu)) return;
    char avail[MAX_PATH_LEN], max_path[MAX_PATH_LEN], min_path[MAX_PATH_LEN];
    snprintf(avail, sizeof(avail), "%s/gpu_available_frequencies", gpu);
    snprintf(max_path, sizeof(max_path), "%s/gpu_max_clock", gpu);
    snprintf(min_path, sizeof(min_path), "%s/gpu_min_clock", gpu);
    long max = get_max_freq(avail);
    long min = get_min_freq(avail);
    long mid = get_mid_freq(avail);
    if (mode == 1) {
        apply_ll(max, max_path);
        apply_ll(LITE_MODE == 1 && mid > 0 ? mid : max, min_path);
    } else if (mode == 2) {
        write_ll(max, max_path);
        write_ll(min, min_path);
    } else {
        apply_ll(min, min_path);
        apply_ll(min, max_path);
    }
}

typedef struct { int mode; int once; } mali_ctx;

static bool mali_name(const char *name, void *ctx) {
    (void)ctx;
    return name_contains(name, ".mali");
}

static void exynos_mali_handler(const char *dir, const char *name, void *ctx) {
    mali_ctx *data = ctx;
    if (data->once) return;
    char path[MAX_PATH_LEN];
    if (path_join3(path, sizeof(path), dir, name, "power_policy")) apply(data->mode == 1 ? "always_on" : "coarse_demand", path);
    if (path_join3(path, sizeof(path), dir, name, "dvfs_period")) apply(data->mode == 1 ? "1" : data->mode == 2 ? "16" : "32", path);
    data->once = 1;
}

static bool mif_name(const char *name, void *ctx) {
    (void)ctx;
    return name_contains(name, "devfreq_mif") || name_contains(name, "devfreq_int");
}

static void mif_handler(const char *dir, const char *name, void *ctx) {
    int mode = *(int *)ctx;
    char path[MAX_PATH_LEN];
    if (!path_join(path, sizeof(path), dir, name)) return;
    if (mode == 1) LITE_MODE == 1 ? devfreq_mid_perf(path) : devfreq_max_perf(path);
    else if (mode == 2) devfreq_unlock(path);
    else devfreq_min_perf(path);
}

static void exynos_bus_governor(int mode) {
    const char *gov = mode == 1 ? "performance" : mode == 2 ? "simple_ondemand" : "powersave";
    apply(gov, "/sys/class/devfreq/17000010.devfreq_mif/scaling_devfreq_governor");
    apply(gov, "/sys/class/devfreq/17000020.devfreq_int/scaling_devfreq_governor");
    char asv[MAX_LINE_LEN];
    read_string_from_file(asv, sizeof(asv), "/sys/devices/platform/exynos-asv/asv_table");
}

static void exynos_mode(int mode) {
    exynos_kernel_gpu(mode);
    mali_ctx ctx = {.mode = mode, .once = 0};
    for_each_dir_entry("/sys/devices/platform", mali_name, exynos_mali_handler, &ctx);
    if (DEVICE_MITIGATION == 0) for_each_dir_entry("/sys/class/devfreq", mif_name, mif_handler, &(int){mode});
    exynos_bus_governor(mode);
}

void exynos_esport(void) { exynos_mode(1); }
void exynos_balanced(void) { exynos_mode(2); }
void exynos_efficiency(void) { exynos_mode(3); }
