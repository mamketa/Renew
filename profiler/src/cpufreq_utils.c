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

typedef struct {
    const char *gov;
    int count;
} policy_gov_ctx;

int get_cpu_count(void) {
    int count = 0;
    DIR *dir = opendir("/sys/devices/system/cpu");
    if (!dir) return 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, "cpu", 3) == 0 && isdigit((unsigned char)ent->d_name[3])) ++count;
    }
    closedir(dir);
    return count;
}

static void policy_gov_handler(const char *dir, const char *name, void *ctx) {
    policy_gov_ctx *data = ctx;
    char path[MAX_PATH_LEN];
    if (path_join3(path, sizeof(path), dir, name, "scaling_governor")) {
        write_file(data->gov, path);
        ++data->count;
    }
}

void change_cpu_gov(const char *gov) {
    if (!gov || !gov[0]) return;
    int cpu_count = get_cpu_count();
    for (int i = 0; i < cpu_count; ++i) {
        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor", i);
        write_file(gov, path);
    }
    policy_gov_ctx ctx = {.gov = gov, .count = 0};
    for_each_dir_entry("/sys/devices/system/cpu/cpufreq", name_is_policy, policy_gov_handler, &ctx);
}

long get_max_freq(const char *path) {
    FILE *fp = path ? fopen(path, "r") : NULL;
    if (!fp) return 0;
    long max_freq = 0, freq = 0;
    while (fscanf(fp, "%ld", &freq) == 1) if (freq > max_freq) max_freq = freq;
    fclose(fp);
    return max_freq;
}

long get_min_freq(const char *path) {
    FILE *fp = path ? fopen(path, "r") : NULL;
    if (!fp) return 0;
    long min_freq = LONG_MAX, freq = 0;
    while (fscanf(fp, "%ld", &freq) == 1) if (freq > 0 && freq < min_freq) min_freq = freq;
    fclose(fp);
    return min_freq == LONG_MAX ? 0 : min_freq;
}

long get_mid_freq(const char *path) {
    FILE *fp = path ? fopen(path, "r") : NULL;
    if (!fp) return 0;
    long freqs[MAX_OPP_COUNT];
    int count = 0;
    while (count < MAX_OPP_COUNT && fscanf(fp, "%ld", &freqs[count]) == 1) ++count;
    fclose(fp);
    if (count == 0) return 0;
    for (int i = 1; i < count; ++i) {
        long v = freqs[i];
        int j = i - 1;
        while (j >= 0 && freqs[j] > v) {
            freqs[j + 1] = freqs[j];
            --j;
        }
        freqs[j + 1] = v;
    }
    return freqs[count / 2];
}

static void ppm_max_handler(const char *dir, const char *name, void *ctx) {
    int *cluster = ctx;
    char path[MAX_PATH_LEN];
    char cmd[64];
    if (!path_join3(path, sizeof(path), dir, name, "cpuinfo_max_freq")) return;
    long max_freq = read_int_from_file(path);
    snprintf(cmd, sizeof(cmd), "%d %ld", *cluster, max_freq);
    apply(cmd, "/proc/ppm/policy/hard_userlimit_max_cpu_freq");
    if (LITE_MODE == 1) {
        path_join3(path, sizeof(path), dir, name, "scaling_available_frequencies");
        long mid_freq = get_mid_freq(path);
        snprintf(cmd, sizeof(cmd), "%d %ld", *cluster, mid_freq > 0 ? mid_freq : max_freq);
    }
    apply(cmd, "/proc/ppm/policy/hard_userlimit_min_cpu_freq");
    ++*cluster;
}

void cpufreq_ppm_max_perf(void) {
    int cluster = 0;
    for_each_dir_entry("/sys/devices/system/cpu/cpufreq", name_is_policy, ppm_max_handler, &cluster);
}

void cpufreq_max_perf(void) {
    int cpu_count = get_cpu_count();
    for (int i = 0; i < cpu_count; ++i) {
        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", i);
        long max_freq = read_int_from_file(path);
        if (max_freq <= 0) continue;
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
        apply_ll(max_freq, path);
        long min_target = max_freq;
        if (LITE_MODE == 1) {
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_available_frequencies", i);
            long mid = get_mid_freq(path);
            if (mid > 0) min_target = mid;
        }
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
        apply_ll(min_target, path);
    }
}

static void ppm_unlock_handler(const char *dir, const char *name, void *ctx) {
    int *cluster = ctx;
    char path[MAX_PATH_LEN];
    char cmd[64];
    path_join3(path, sizeof(path), dir, name, "cpuinfo_max_freq");
    long max_freq = read_int_from_file(path);
    path_join3(path, sizeof(path), dir, name, "cpuinfo_min_freq");
    long min_freq = read_int_from_file(path);
    snprintf(cmd, sizeof(cmd), "%d %ld", *cluster, max_freq);
    write_file(cmd, "/proc/ppm/policy/hard_userlimit_max_cpu_freq");
    snprintf(cmd, sizeof(cmd), "%d %ld", *cluster, min_freq);
    write_file(cmd, "/proc/ppm/policy/hard_userlimit_min_cpu_freq");
    ++*cluster;
}

void cpufreq_ppm_unlock(void) {
    int cluster = 0;
    for_each_dir_entry("/sys/devices/system/cpu/cpufreq", name_is_policy, ppm_unlock_handler, &cluster);
}

void cpufreq_unlock(void) {
    int cpu_count = get_cpu_count();
    for (int i = 0; i < cpu_count; ++i) {
        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", i);
        long max_freq = read_int_from_file(path);
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_min_freq", i);
        long min_freq = read_int_from_file(path);
        if (max_freq <= 0 || min_freq <= 0) continue;
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
        write_ll(max_freq, path);
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
        write_ll(min_freq, path);
    }
}
