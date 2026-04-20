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

/* Returns cached CPU core count (scans once at first call) */
int  get_cpu_count(void);

/* Frequency readers (operate on a sysfs available_frequencies file) */
long get_max_freq(const char *avail_freq_path);
long get_min_freq(const char *avail_freq_path);
long get_mid_freq(const char *avail_freq_path);

/* Governor control — applies to all per-core and policy nodes */
void change_cpu_gov(const char *gov);

/* PPM-aware max performance (MediaTek PPM) */
void cpufreq_ppm_max_perf(void);

/* Generic max performance (locks min == max at cpuinfo_max_freq) */
void cpufreq_max_perf(void);

/* PPM-aware unlock (restores natural min/max) */
void cpufreq_ppm_unlock(void);

/* Generic unlock (restores cpuinfo_min/max) */
void cpufreq_unlock(void);
