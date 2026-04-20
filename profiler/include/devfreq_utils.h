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

/* Device class constants for find_devfreq_devices */
#define DEVFREQ_CLASS_GPU   0
#define DEVFREQ_CLASS_MIF   1
#define DEVFREQ_CLASS_INT   2
#define DEVFREQ_CLASS_UFS   3
#define DEVFREQ_CLASS_OTHER 4

typedef struct {
    char path[MAX_PATH_LEN];
    int  devclass;
} DevfreqDevice;

/*
 * find_devfreq_devices:
 * Scans /sys/class/devfreq/, classifies each entry, fills `out` up to `max`.
 * Returns number of devices found.
 * Avoids repeated scans — call once per mode and reuse results.
 */
int find_devfreq_devices(DevfreqDevice *out, int max);

/* Standard devfreq perf helpers */
int devfreq_max_perf(const char *path);
int devfreq_mid_perf(const char *path);
int devfreq_min_perf(const char *path);
int devfreq_unlock(const char *path);

/* Qualcomm bus_dcvs helpers (hw_min_freq / hw_max_freq) */
int qcom_cpudcvs_max_perf(const char *path);
int qcom_cpudcvs_mid_perf(const char *path);
int qcom_cpudcvs_min_perf(const char *path);
int qcom_cpudcvs_unlock(const char *path);

/* MediaTek GPU OPP index parsers */
int mtk_gpufreq_minfreq_index(const char *opp_table_path);
int mtk_gpufreq_midfreq_index(const char *opp_table_path);
