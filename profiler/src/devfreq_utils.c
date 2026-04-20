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

#include "devfreq_utils.h"
#include "cpu_utils.h"
#include "utility_utils.h"

/* Classify a devfreq entry name into a device class */
static int _classify_devfreq(const char *name) {
    if (strstr(name, "kgsl") || strstr(name, "mali") || strstr(name, ".gpu"))
        return DEVFREQ_CLASS_GPU;
    if (strstr(name, "mif") || strstr(name, "ddr") || strstr(name, "memlat") ||
        strstr(name, "llcc") || strstr(name, "cpubw") || strstr(name, "cpu-bw") ||
        strstr(name, "cpu-lat") || strstr(name, "kgsl-ddr-qos"))
        return DEVFREQ_CLASS_MIF;
    if (strstr(name, "int") || strstr(name, "bus_int") || strstr(name, "devfreq_int"))
        return DEVFREQ_CLASS_INT;
    if (strstr(name, "ufshc") || strstr(name, "mmc"))
        return DEVFREQ_CLASS_UFS;
    return DEVFREQ_CLASS_OTHER;
}

int find_devfreq_devices(DevfreqDevice *out, int max) {
    if (!out || max <= 0) return 0;
    DIR *dir = opendir("/sys/class/devfreq");
    if (!dir) return 0;
    int count = 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL && count < max) {
        if (ent->d_name[0] == '.') continue;
        snprintf(out[count].path, MAX_PATH_LEN,
                 "/sys/class/devfreq/%s", ent->d_name);
        out[count].devclass = _classify_devfreq(ent->d_name);
        count++;
    }
    closedir(dir);
    return count;
}

int devfreq_max_perf(const char *path) {
    if (!path || !node_exists(path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", path);
    if (!node_exists(freq_path)) return 0;
    long max_freq = get_max_freq(freq_path);
    if (max_freq <= 0) return 0;
    char p[MAX_PATH_LEN];
    snprintf(p, sizeof(p), "%s/max_freq", path);
    apply_ll(max_freq, p);
    snprintf(p, sizeof(p), "%s/min_freq", path);
    apply_ll(max_freq, p);
    return 1;
}

int devfreq_mid_perf(const char *path) {
    if (!path || !node_exists(path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", path);
    if (!node_exists(freq_path)) return 0;
    long max_freq = get_max_freq(freq_path);
    long mid_freq = get_mid_freq(freq_path);
    if (max_freq <= 0 || mid_freq <= 0) return 0;
    char p[MAX_PATH_LEN];
    snprintf(p, sizeof(p), "%s/max_freq", path);
    apply_ll(max_freq, p);
    snprintf(p, sizeof(p), "%s/min_freq", path);
    apply_ll(mid_freq, p);
    return 1;
}

int devfreq_min_perf(const char *path) {
    if (!path || !node_exists(path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", path);
    if (!node_exists(freq_path)) return 0;
    long freq = get_min_freq(freq_path);
    if (freq <= 0) return 0;
    char p[MAX_PATH_LEN];
    snprintf(p, sizeof(p), "%s/min_freq", path);
    apply_ll(freq, p);
    snprintf(p, sizeof(p), "%s/max_freq", path);
    apply_ll(freq, p);
    return 1;
}

int devfreq_unlock(const char *path) {
    if (!path || !node_exists(path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", path);
    if (!node_exists(freq_path)) return 0;
    long max_freq = get_max_freq(freq_path);
    long min_freq = get_min_freq(freq_path);
    if (max_freq <= 0 || min_freq <= 0) return 0;
    char p[MAX_PATH_LEN];
    snprintf(p, sizeof(p), "%s/max_freq", path);
    write_ll(max_freq, p);
    snprintf(p, sizeof(p), "%s/min_freq", path);
    write_ll(min_freq, p);
    return 1;
}

int qcom_cpudcvs_max_perf(const char *path) {
    if (!node_exists(path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", path);
    if (!node_exists(freq_path)) return 0;
    long freq = get_max_freq(freq_path);
    char p[MAX_PATH_LEN];
    snprintf(p, sizeof(p), "%s/hw_max_freq", path);
    apply_ll(freq, p);
    snprintf(p, sizeof(p), "%s/hw_min_freq", path);
    apply_ll(freq, p);
    return 1;
}

int qcom_cpudcvs_mid_perf(const char *path) {
    if (!node_exists(path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", path);
    if (!node_exists(freq_path)) return 0;
    long max_freq = get_max_freq(freq_path);
    long mid_freq = get_mid_freq(freq_path);
    char p[MAX_PATH_LEN];
    snprintf(p, sizeof(p), "%s/hw_max_freq", path);
    apply_ll(max_freq, p);
    snprintf(p, sizeof(p), "%s/hw_min_freq", path);
    apply_ll(mid_freq, p);
    return 1;
}

int qcom_cpudcvs_min_perf(const char *path) {
    if (!node_exists(path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", path);
    if (!node_exists(freq_path)) return 0;
    long freq = get_min_freq(freq_path);
    char p[MAX_PATH_LEN];
    snprintf(p, sizeof(p), "%s/hw_min_freq", path);
    apply_ll(freq, p);
    snprintf(p, sizeof(p), "%s/hw_max_freq", path);
    apply_ll(freq, p);
    return 1;
}

int qcom_cpudcvs_unlock(const char *path) {
    if (!node_exists(path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", path);
    if (!node_exists(freq_path)) return 0;
    long max_freq = get_max_freq(freq_path);
    long min_freq = get_min_freq(freq_path);
    char p[MAX_PATH_LEN];
    snprintf(p, sizeof(p), "%s/hw_max_freq", path);
    write_ll(max_freq, p);
    snprintf(p, sizeof(p), "%s/hw_min_freq", path);
    write_ll(min_freq, p);
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
            int len = (int)(end - start - 1);
            if (len > 0 && len < (int)sizeof(tmp)) {
                memcpy(tmp, start + 1, (size_t)len);
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
    while (fgets(line, sizeof(line), fp) && count < MAX_OPP_COUNT) {
        char *start = strchr(line, '[');
        char *end   = strchr(line, ']');
        if (start && end && end > start) {
            char tmp[16];
            int len = (int)(end - start - 1);
            if (len > 0 && len < (int)sizeof(tmp)) {
                memcpy(tmp, start + 1, (size_t)len);
                tmp[len] = '\0';
                indices[count++] = atoi(tmp);
            }
        }
    }
    fclose(fp);
    return (count > 0) ? indices[count / 2] : 0;
}
