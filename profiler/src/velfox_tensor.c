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

typedef struct { int mode; int once; } tensor_gpu_ctx;

static bool mali_name(const char *name, void *ctx) {
    (void)ctx;
    return name_contains(name, ".mali");
}

static void tensor_mali_handler(const char *dir, const char *name, void *ctx) {
    tensor_gpu_ctx *data = ctx;
    if (data->once) return;
    char gpu[MAX_PATH_LEN], avail[MAX_PATH_LEN], max_path[MAX_PATH_LEN], min_path[MAX_PATH_LEN], policy[MAX_PATH_LEN], period[MAX_PATH_LEN];
    if (!path_join(gpu, sizeof(gpu), dir, name)) return;
    snprintf(avail, sizeof(avail), "%s/available_frequencies", gpu);
    snprintf(max_path, sizeof(max_path), "%s/scaling_max_freq", gpu);
    snprintf(min_path, sizeof(min_path), "%s/scaling_min_freq", gpu);
    snprintf(policy, sizeof(policy), "%s/power_policy", gpu);
    snprintf(period, sizeof(period), "%s/dvfs_period", gpu);
    long max = get_max_freq(avail);
    long min = get_min_freq(avail);
    long mid = get_mid_freq(avail);
    if (data->mode == 1) {
        apply_ll(max, max_path);
        apply_ll(LITE_MODE == 1 && mid > 0 ? mid : max, min_path);
    } else if (data->mode == 2) {
        write_ll(max, max_path);
        write_ll(min, min_path);
    } else {
        apply_ll(min, min_path);
        apply_ll(min, max_path);
    }
    apply(data->mode == 1 ? "always_on" : "coarse_demand", policy);
    apply(data->mode == 1 ? "1" : data->mode == 2 ? "16" : "32", period);
    data->once = 1;
}

static bool mif_name(const char *name, void *ctx) {
    (void)ctx;
    return name_contains(name, "devfreq_mif");
}

static void tensor_mif_handler(const char *dir, const char *name, void *ctx) {
    int mode = *(int *)ctx;
    char path[MAX_PATH_LEN];
    if (!path_join(path, sizeof(path), dir, name)) return;
    if (mode == 1) LITE_MODE == 1 ? devfreq_mid_perf(path) : devfreq_max_perf(path);
    else if (mode == 2) devfreq_unlock(path);
    else devfreq_min_perf(path);
}

static void tensor_latency(int mode) {
    apply(mode == 1 ? "performance" : mode == 2 ? "simple_ondemand" : "powersave", "/sys/devices/platform/soc/soc:bus/devfreq/soc:bus:bus_latency/governor");
}

static void tensor_mode(int mode) {
    tensor_gpu_ctx ctx = {.mode = mode, .once = 0};
    for_each_dir_entry("/sys/devices/platform", mali_name, tensor_mali_handler, &ctx);
    if (DEVICE_MITIGATION == 0) for_each_dir_entry("/sys/class/devfreq", mif_name, tensor_mif_handler, &(int){mode});
    tensor_latency(mode);
}

void tensor_esport(void) { tensor_mode(1); }
void tensor_balanced(void) { tensor_mode(2); }
void tensor_efficiency(void) { tensor_mode(3); }
