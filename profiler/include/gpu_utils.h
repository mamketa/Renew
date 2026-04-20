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

#pragma once

#include "velfox_common.h"

/* GPU path discovery — returns a static internal buffer or NULL */
const char *find_gpu_path(void);

/*
 * find_mali_platform_path:
 * Scans /sys/devices/platform for *.mali entries.
 * Returns pointer to static buffer, or NULL.
 */
const char *find_mali_platform_path(void);

/* Generic GPU devfreq helpers (work with kgsl-3d0/devfreq, mali devfreq, etc.) */
int gpu_max_perf(const char *gpu_devfreq_path);
int gpu_mid_perf(const char *gpu_devfreq_path);
int gpu_min_perf(const char *gpu_devfreq_path);
int gpu_unlock(const char *gpu_devfreq_path);
