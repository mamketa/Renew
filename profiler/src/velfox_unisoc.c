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
#include "devfreq_utils.h"
#include "gpu_utils.h"
#include "utility_utils.h"
#include "velfox_common.h"

/*
 * Unisoc GPU lives in /sys/class/devfreq with a ".gpu" suffix.
 * Reuse find_gpu_path() which already handles this pattern.
 */
static void _unisoc_gpu(int mode) {
    const char *gpu = find_gpu_path();
    if (!gpu) return;

    if (mode == 1) {
        if (LITE_MODE == 0) devfreq_max_perf(gpu);
        else                devfreq_mid_perf(gpu);
    } else if (mode == 2) {
        devfreq_unlock(gpu);
    } else {
        devfreq_min_perf(gpu);
    }
}

void unisoc_esport(void) {
    _unisoc_gpu(1);
}

void unisoc_balanced(void) {
    _unisoc_gpu(2);
}

void unisoc_efficiency(void) {
    _unisoc_gpu(3);
}
