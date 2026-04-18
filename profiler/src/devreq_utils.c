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

int devfreq_max_perf(const char *path) {
    if (!path || !file_exists(path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path),
             "%s/available_frequencies", path);
    if (!file_exists(freq_path)) return 0;
    long max_freq = get_max_freq(freq_path);
    if (max_freq <= 0) return 0;
    char max_path[MAX_PATH_LEN];
    char min_path[MAX_PATH_LEN];
    snprintf(max_path, sizeof(max_path), "%s/max_freq", path);
    snprintf(min_path, sizeof(min_path), "%s/min_freq", path);
    apply_ll(max_freq, max_path);
    apply_ll(max_freq, min_path);

    return 1;
}

int devfreq_mid_perf(const char *path) {
    if (!path || !file_exists(path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path),
             "%s/available_frequencies", path);
    if (!file_exists(freq_path)) return 0;
    long max_freq = get_max_freq(freq_path);
    long mid_freq = get_mid_freq(freq_path);
    if (max_freq <= 0 || mid_freq <= 0) return 0;
    char max_path[MAX_PATH_LEN];
    char min_path[MAX_PATH_LEN];
    snprintf(max_path, sizeof(max_path), "%s/max_freq", path);
    snprintf(min_path, sizeof(min_path), "%s/min_freq", path);
    apply_ll(max_freq, max_path);
    apply_ll(mid_freq, min_path);

    return 1;
}

int devfreq_unlock(const char *path) {
    if (!path || !file_exists(path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path),
             "%s/available_frequencies", path);
    if (!file_exists(freq_path)) return 0;
    long max_freq = get_max_freq(freq_path);
    long min_freq = get_min_freq(freq_path);
    if (max_freq <= 0 || min_freq <= 0) return 0;
    char max_path[MAX_PATH_LEN];
    char min_path[MAX_PATH_LEN];
    snprintf(max_path, sizeof(max_path), "%s/max_freq", path);
    snprintf(min_path, sizeof(min_path), "%s/min_freq", path);
    write_ll(max_freq, max_path);
    write_ll(min_freq, min_path);

    return 1;
}

int devfreq_min_perf(const char *path) {
    if (!path || !file_exists(path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path),
             "%s/available_frequencies", path);
    if (!file_exists(freq_path)) return 0;
    long freq = get_min_freq(freq_path);
    if (freq <= 0) return 0;
    char min_path[MAX_PATH_LEN];
    char max_path[MAX_PATH_LEN];
    snprintf(min_path, sizeof(min_path), "%s/min_freq", path);
    snprintf(max_path, sizeof(max_path), "%s/max_freq", path);
    apply_ll(freq, min_path);
    apply_ll(freq, max_path);

    return 1;
}

int qcom_cpudcvs_max_perf(const char *path) {
    if (!file_exists(path)) return 0; 
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", path);
    if (!file_exists(freq_path)) return 0;
    long freq = get_max_freq(freq_path);
    char max_path[MAX_PATH_LEN];
    snprintf(max_path, sizeof(max_path), "%s/hw_max_freq", path);
    apply_ll(freq, max_path);
    char min_path[MAX_PATH_LEN];
    snprintf(min_path, sizeof(min_path), "%s/hw_min_freq", path);
    apply_ll(freq, min_path);
    
    return 1;
}

int qcom_cpudcvs_mid_perf(const char *path) {
    if (!file_exists(path)) return 0; 
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", path);
    if (!file_exists(freq_path)) return 0;
    long max_freq = get_max_freq(freq_path);
    long mid_freq = get_mid_freq(freq_path);
    char max_path[MAX_PATH_LEN];
    snprintf(max_path, sizeof(max_path), "%s/hw_max_freq", path);
    apply_ll(max_freq, max_path);
    char min_path[MAX_PATH_LEN];
    snprintf(min_path, sizeof(min_path), "%s/hw_min_freq", path);
    apply_ll(mid_freq, min_path);
    
    return 1;
}

int qcom_cpudcvs_unlock(const char *path) {
    if (!file_exists(path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", path);
    if (!file_exists(freq_path)) return 0;
    long max_freq = get_max_freq(freq_path);
    long min_freq = get_min_freq(freq_path);
    char max_path[MAX_PATH_LEN];
    snprintf(max_path, sizeof(max_path), "%s/hw_max_freq", path);
    write_ll(max_freq, max_path);
    char min_path[MAX_PATH_LEN];
    snprintf(min_path, sizeof(min_path), "%s/hw_min_freq", path);
    write_ll(min_freq, min_path);
    
    return 1;
}

int qcom_cpudcvs_min_perf(const char *path) {
    if (!file_exists(path)) return 0; 
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", path);
    if (!file_exists(freq_path)) return 0;
    long freq = get_min_freq(freq_path);
    char min_path[MAX_PATH_LEN];
    snprintf(min_path, sizeof(min_path), "%s/hw_min_freq", path);
    apply_ll(freq, min_path);
    char max_path[MAX_PATH_LEN];
    snprintf(max_path, sizeof(max_path), "%s/hw_max_freq", path);
    apply_ll(freq, max_path);
    
    return 1;
}

int mtk_gpufreq_minfreq_index(const char *path) {
    if (!path) return 0;
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    char line[MAX_LINE_LEN];
    int min_index = 0;
    while (fgets(line, sizeof(line), fp)) {
        char *start = strchr(line, '[');
        char *end   = strchr(line, ']');
        if (start && end && end > start) {
            char tmp[16];
            int len = end - start - 1;
            if (len > 0 && len < (int)sizeof(tmp)) {
                memcpy(tmp, start + 1, len);
                tmp[len] = '\0';
                min_index = atoi(tmp);
            }
        }
    }
    fclose(fp);
    return min_index;
}

int mtk_gpufreq_midfreq_index(const char *path) {
    if (!path) return 0;
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    char line[MAX_LINE_LEN];
    int indices[MAX_OPP_COUNT];
    int count = 0;
    while (fgets(line, sizeof(line), fp) &&
           count < MAX_OPP_COUNT) {
        char *start = strchr(line, '[');
        char *end   = strchr(line, ']');
        if (start && end && end > start) {
            char tmp[16];
            int len = end - start - 1;
            if (len > 0 && len < (int)sizeof(tmp)) {
                memcpy(tmp, start + 1, len);
                tmp[len] = '\0';
                indices[count++] = atoi(tmp);
            }
        }
    }
    fclose(fp);
    if (count == 0) return 0;
    return indices[count / 2];
}
