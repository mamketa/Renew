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

#include "cpu_utils.h"
#include "utility_utils.h"
#include "velfox_common.h"

/* Cached CPU count — scanned exactly once */
static int _cpu_count_cache = -1;

int get_cpu_count(void) {
    if (_cpu_count_cache >= 0)
        return _cpu_count_cache;

    int count = 0;
    DIR *dir = opendir("/sys/devices/system/cpu");
    if (!dir) {
        _cpu_count_cache = 0;
        return 0;
    }
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, "cpu", 3) == 0 && isdigit((unsigned char)ent->d_name[3]))
            count++;
    }
    closedir(dir);
    _cpu_count_cache = count;
    return count;
}

long get_max_freq(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    long max_freq = 0, freq;
    while (fscanf(fp, "%ld", &freq) == 1) {
        if (freq > max_freq) max_freq = freq;
    }
    fclose(fp);
    return max_freq;
}

long get_min_freq(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    long min_freq = LONG_MAX, freq;
    while (fscanf(fp, "%ld", &freq) == 1) {
        if (freq > 0 && freq < min_freq) min_freq = freq;
    }
    fclose(fp);
    return (min_freq == LONG_MAX) ? 0 : min_freq;
}

long get_mid_freq(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    long freqs[MAX_OPP_COUNT];
    int count = 0;
    while (count < MAX_OPP_COUNT && fscanf(fp, "%ld", &freqs[count]) == 1)
        count++;
    fclose(fp);
    if (count == 0) return 0;

    /* Insertion sort (small array, avoids stdlib dependency on qsort overhead) */
    for (int i = 1; i < count; i++) {
        long key = freqs[i];
        int j = i - 1;
        while (j >= 0 && freqs[j] > key) {
            freqs[j + 1] = freqs[j];
            j--;
        }
        freqs[j + 1] = key;
    }
    return freqs[count / 2];
}

void change_cpu_gov(const char *gov) {
    char path[MAX_PATH_LEN];
    int cpu_count = get_cpu_count();

    /* Per-core nodes */
    for (int i = 0; i < cpu_count; i++) {
        snprintf(path, sizeof(path),
                 "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor", i);
        safe_write(path, gov);
    }

    /* Policy nodes */
    DIR *dir = opendir("/sys/devices/system/cpu/cpufreq");
    if (!dir) return;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (!strstr(ent->d_name, "policy")) continue;
        snprintf(path, sizeof(path),
                 "/sys/devices/system/cpu/cpufreq/%s/scaling_governor",
                 ent->d_name);
        safe_write(path, gov);
    }
    closedir(dir);
}

void cpufreq_ppm_max_perf(void) {
    DIR *dir = opendir("/sys/devices/system/cpu/cpufreq");
    if (!dir) return;
    struct dirent *ent;
    char path[MAX_PATH_LEN];
    int cluster = 0;

    while ((ent = readdir(dir)) != NULL) {
        if (!strstr(ent->d_name, "policy")) continue;

        snprintf(path, sizeof(path),
                 "/sys/devices/system/cpu/cpufreq/%s/cpuinfo_max_freq",
                 ent->d_name);
        long cpu_maxfreq = read_int_from_file(path);

        char ppm_cmd[128];
        if (node_exists("/proc/ppm/policy/hard_userlimit_max_cpu_freq")) {
            snprintf(ppm_cmd, sizeof(ppm_cmd), "%d %ld", cluster, cpu_maxfreq);
            apply(ppm_cmd, "/proc/ppm/policy/hard_userlimit_max_cpu_freq");
        }
        if (node_exists("/proc/ppm/policy/hard_userlimit_min_cpu_freq")) {
            long limit_freq = cpu_maxfreq;
            if (LITE_MODE == 1) {
                snprintf(path, sizeof(path),
                         "/sys/devices/system/cpu/cpufreq/%s/scaling_available_frequencies",
                         ent->d_name);
                long mid = get_mid_freq(path);
                if (mid > 0) limit_freq = mid;
            }
            snprintf(ppm_cmd, sizeof(ppm_cmd), "%d %ld", cluster, limit_freq);
            apply(ppm_cmd, "/proc/ppm/policy/hard_userlimit_min_cpu_freq");
        }
        cluster++;
    }
    closedir(dir);
}

void cpufreq_max_perf(void) {
    char path[MAX_PATH_LEN];
    int cpu_count = get_cpu_count();

    for (int i = 0; i < cpu_count; i++) {
        snprintf(path, sizeof(path),
                 "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", i);
        if (!node_exists(path)) continue;

        long cpu_maxfreq = read_int_from_file(path);

        snprintf(path, sizeof(path),
                 "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
        apply_ll(cpu_maxfreq, path);

        long min_target = cpu_maxfreq;
        if (LITE_MODE == 1) {
            snprintf(path, sizeof(path),
                     "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_available_frequencies", i);
            long mid = get_mid_freq(path);
            if (mid > 0) min_target = mid;
        }
        snprintf(path, sizeof(path),
                 "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
        apply_ll(min_target, path);
    }
}

void cpufreq_ppm_unlock(void) {
    DIR *dir = opendir("/sys/devices/system/cpu/cpufreq");
    if (!dir) return;
    struct dirent *ent;
    char path[MAX_PATH_LEN];
    int cluster = 0;

    while ((ent = readdir(dir)) != NULL) {
        if (!strstr(ent->d_name, "policy")) continue;

        snprintf(path, sizeof(path),
                 "/sys/devices/system/cpu/cpufreq/%s/cpuinfo_max_freq",
                 ent->d_name);
        long cpu_maxfreq = read_int_from_file(path);
        snprintf(path, sizeof(path),
                 "/sys/devices/system/cpu/cpufreq/%s/cpuinfo_min_freq",
                 ent->d_name);
        long cpu_minfreq = read_int_from_file(path);

        char ppm_cmd[128];
        if (node_exists("/proc/ppm/policy/hard_userlimit_max_cpu_freq")) {
            snprintf(ppm_cmd, sizeof(ppm_cmd), "%d %ld", cluster, cpu_maxfreq);
            write_file(ppm_cmd, "/proc/ppm/policy/hard_userlimit_max_cpu_freq");
        }
        if (node_exists("/proc/ppm/policy/hard_userlimit_min_cpu_freq")) {
            snprintf(ppm_cmd, sizeof(ppm_cmd), "%d %ld", cluster, cpu_minfreq);
            write_file(ppm_cmd, "/proc/ppm/policy/hard_userlimit_min_cpu_freq");
        }
        cluster++;
    }
    closedir(dir);
}

void cpufreq_unlock(void) {
    char path[MAX_PATH_LEN];
    int cpu_count = get_cpu_count();

    for (int i = 0; i < cpu_count; i++) {
        snprintf(path, sizeof(path),
                 "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", i);
        if (!node_exists(path)) continue;

        long cpu_maxfreq = read_int_from_file(path);
        snprintf(path, sizeof(path),
                 "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_min_freq", i);
        long cpu_minfreq = read_int_from_file(path);

        snprintf(path, sizeof(path),
                 "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
        write_ll(cpu_maxfreq, path);

        snprintf(path, sizeof(path),
                 "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
        write_ll(cpu_minfreq, path);
    }
}
