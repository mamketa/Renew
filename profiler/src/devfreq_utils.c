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
 
#include "velfox_common.h"
#include "velfox_cpufreq.h"
#include "velfox_devfreq.h"

static int set_devfreq_pair(const char *path, const char *max_node, const char *min_node, long max_freq, long min_freq, int persistent) {
    if (!path || max_freq <= 0 || min_freq <= 0) return 0;
    char max_path[MAX_PATH_LEN], min_path[MAX_PATH_LEN];
    snprintf(max_path, sizeof(max_path), "%s/%s", path, max_node);
    snprintf(min_path, sizeof(min_path), "%s/%s", path, min_node);
    if (persistent) {
        write_ll(max_freq, max_path);
        write_ll(min_freq, min_path);
    } else {
        apply_ll(max_freq, max_path);
        apply_ll(min_freq, min_path);
    }
    return 1;
}

static int devfreq_set(const char *path, int mode, int persistent) {
    if (!path || !file_exists(path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", path);
    if (!file_exists(freq_path)) return 0;
    long max_freq = get_max_freq(freq_path);
    long min_freq = get_min_freq(freq_path);
    long mid_freq = get_mid_freq(freq_path);
    if (mode == 1) return set_devfreq_pair(path, "max_freq", "min_freq", max_freq, max_freq, persistent);
    if (mode == 2) return set_devfreq_pair(path, "max_freq", "min_freq", max_freq, mid_freq, persistent);
    if (mode == 3) return set_devfreq_pair(path, "max_freq", "min_freq", min_freq, min_freq, persistent);
    return set_devfreq_pair(path, "max_freq", "min_freq", max_freq, min_freq, persistent);
}

int devfreq_max_perf(const char *path) { return devfreq_set(path, 1, 0); }
int devfreq_mid_perf(const char *path) { return devfreq_set(path, 2, 0); }
int devfreq_unlock(const char *path) { return devfreq_set(path, 0, 1); }
int devfreq_min_perf(const char *path) { return devfreq_set(path, 3, 0); }

static int qcom_cpudcvs_set(const char *path, int mode, int persistent) {
    if (!path || !file_exists(path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", path);
    if (!file_exists(freq_path)) return 0;
    long max_freq = get_max_freq(freq_path);
    long min_freq = get_min_freq(freq_path);
    long mid_freq = get_mid_freq(freq_path);
    if (mode == 1) return set_devfreq_pair(path, "hw_max_freq", "hw_min_freq", max_freq, max_freq, persistent);
    if (mode == 2) return set_devfreq_pair(path, "hw_max_freq", "hw_min_freq", max_freq, mid_freq, persistent);
    if (mode == 3) return set_devfreq_pair(path, "hw_max_freq", "hw_min_freq", min_freq, min_freq, persistent);
    return set_devfreq_pair(path, "hw_max_freq", "hw_min_freq", max_freq, min_freq, persistent);
}

int qcom_cpudcvs_max_perf(const char *path) { return qcom_cpudcvs_set(path, 1, 0); }
int qcom_cpudcvs_mid_perf(const char *path) { return qcom_cpudcvs_set(path, 2, 0); }
int qcom_cpudcvs_unlock(const char *path) { return qcom_cpudcvs_set(path, 0, 1); }
int qcom_cpudcvs_min_perf(const char *path) { return qcom_cpudcvs_set(path, 3, 0); }

static int parse_mtk_indices(const char *path, int *indices, int limit) {
    FILE *fp = path ? fopen(path, "r") : NULL;
    if (!fp) return 0;
    char line[MAX_LINE_LEN];
    int count = 0;
    while (count < limit && fgets(line, sizeof(line), fp)) {
        char *start = strchr(line, '[');
        char *end = strchr(line, ']');
        if (!start || !end || end <= start) continue;
        char tmp[16];
        int len = (int)(end - start - 1);
        if (len <= 0 || len >= (int)sizeof(tmp)) continue;
        memcpy(tmp, start + 1, (size_t)len);
        tmp[len] = '\0';
        indices[count++] = atoi(tmp);
    }
    fclose(fp);
    return count;
}

int mtk_gpufreq_minfreq_index(const char *path) {
    int indices[MAX_OPP_COUNT];
    int count = parse_mtk_indices(path, indices, MAX_OPP_COUNT);
    return count > 0 ? indices[count - 1] : 0;
}

int mtk_gpufreq_midfreq_index(const char *path) {
    int indices[MAX_OPP_COUNT];
    int count = parse_mtk_indices(path, indices, MAX_OPP_COUNT);
    return count > 0 ? indices[count / 2] : 0;
}
