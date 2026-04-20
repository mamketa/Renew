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

typedef struct { int mode; int once; } gpu_ctx;

static bool unisoc_gpu_name(const char *name, void *ctx) {
    (void)ctx;
    return name_contains(name, ".gpu");
}

static void unisoc_gpu_handler(const char *dir, const char *name, void *ctx) {
    gpu_ctx *data = ctx;
    if (data->once) return;
    char path[MAX_PATH_LEN];
    if (!path_join(path, sizeof(path), dir, name)) return;
    if (data->mode == 1) LITE_MODE == 0 ? devfreq_max_perf(path) : devfreq_mid_perf(path);
    else if (data->mode == 2) devfreq_unlock(path);
    else devfreq_min_perf(path);
    data->once = 1;
}

static bool mali_name(const char *name, void *ctx) {
    (void)ctx;
    return name_contains(name, ".mali");
}

static void mali_platform_handler(const char *dir, const char *name, void *ctx) {
    int mode = *(int *)ctx;
    char path[MAX_PATH_LEN];
    if (path_join3(path, sizeof(path), dir, name, "power_policy")) apply(mode == 1 ? "always_on" : "coarse_demand", path);
    if (path_join3(path, sizeof(path), dir, name, "dvfs_period")) apply(mode == 1 ? "1" : mode == 2 ? "16" : "32", path);
}

static void unisoc_latency(int mode) {
    apply(mode == 1 ? "performance" : mode == 2 ? "simple_ondemand" : "powersave", "/sys/devices/platform/soc/soc:bus/devfreq/soc:bus:bus_latency/governor");
}

static void unisoc_mode(int mode) {
    gpu_ctx ctx = {.mode = mode, .once = 0};
    for_each_dir_entry("/sys/class/devfreq", unisoc_gpu_name, unisoc_gpu_handler, &ctx);
    for_each_dir_entry("/sys/devices/platform", mali_name, mali_platform_handler, &(int){mode});
    unisoc_latency(mode);
}

void unisoc_esport(void) { unisoc_mode(1); }
void unisoc_balanced(void) { unisoc_mode(2); }
void unisoc_efficiency(void) { unisoc_mode(3); }
