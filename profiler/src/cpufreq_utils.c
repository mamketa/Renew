#include "velfox_common.h"
#include "velfox_cpufreq.h"

int get_cpu_count() {
    int count = 0;
    DIR *dir = opendir("/sys/devices/system/cpu");
    if (!dir) return 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, "cpu", 3) == 0 && isdigit((unsigned char)ent->d_name[3])) count++;
    }
    closedir(dir);
    return count;
}

static int is_policy_name(const char *name) {
    return name && strncmp(name, "policy", 6) == 0 && isdigit((unsigned char)name[6]);
}

static int write_policy_governor_cb(const char *entry_path, const char *entry_name, void *userdata) {
    (void)entry_name;
    const char *gov = (const char *)userdata;
    char path[MAX_PATH_LEN];
    if (!path_join(path, sizeof(path), entry_path, "scaling_governor")) return 0;
    return write_sysfs(path, gov);
}

void change_cpu_gov(const char *gov) {
    if (!gov || !gov[0]) return;
    int policy_count = iterate_sysfs_nodes("/sys/devices/system/cpu/cpufreq", "policy", write_policy_governor_cb, (void *)gov);
    if (policy_count > 0) return;
    int cpu_count = get_cpu_count();
    for (int i = 0; i < cpu_count; i++) {
        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor", i);
        write_sysfs(path, gov);
    }
}

long get_max_freq(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    long max_freq = 0;
    long freq;
    while (fscanf(fp, "%ld", &freq) == 1) {
        if (freq > max_freq) max_freq = freq;
    }
    fclose(fp);
    return max_freq;
}

long get_min_freq(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    long min_freq = LONG_MAX;
    long freq;
    while (fscanf(fp, "%ld", &freq) == 1) {
        if (freq > 0 && freq < min_freq) min_freq = freq;
    }
    fclose(fp);
    return min_freq == LONG_MAX ? 0 : min_freq;
}

long get_mid_freq(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    long freqs[MAX_OPP_COUNT];
    int count = 0;
    while (count < MAX_OPP_COUNT && fscanf(fp, "%ld", &freqs[count]) == 1) count++;
    fclose(fp);
    if (count == 0) return 0;
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (freqs[j] > freqs[j + 1]) {
                long tmp = freqs[j];
                freqs[j] = freqs[j + 1];
                freqs[j + 1] = tmp;
            }
        }
    }
    return freqs[count / 2];
}

static long read_policy_freq(const char *entry_path, const char *leaf) {
    char path[MAX_PATH_LEN];
    if (!path_join(path, sizeof(path), entry_path, leaf)) return 0;
    return read_int_from_file(path);
}

static long policy_available_freq(const char *entry_path, int level, long fallback) {
    char path[MAX_PATH_LEN];
    if (!path_join(path, sizeof(path), entry_path, "scaling_available_frequencies")) return fallback;
    long freq = 0;
    if (level > 0) freq = get_max_freq(path);
    else if (level < 0) freq = get_min_freq(path);
    else freq = get_mid_freq(path);
    return freq > 0 ? freq : fallback;
}

struct ppm_policy_data {
    int mode;
    int cluster;
};

static int ppm_policy_cb(const char *entry_path, const char *entry_name, void *userdata) {
    if (!is_policy_name(entry_name)) return 0;
    struct ppm_policy_data *data = (struct ppm_policy_data *)userdata;
    long cpu_maxfreq = read_policy_freq(entry_path, "cpuinfo_max_freq");
    long cpu_minfreq = read_policy_freq(entry_path, "cpuinfo_min_freq");
    if (cpu_maxfreq <= 0) return 0;
    long min_target = cpu_minfreq > 0 ? cpu_minfreq : cpu_maxfreq;
    if (data->mode == 1) min_target = LITE_MODE == 1 ? policy_available_freq(entry_path, 0, cpu_maxfreq) : cpu_maxfreq;
    char ppm_cmd[128];
    snprintf(ppm_cmd, sizeof(ppm_cmd), "%d %ld", data->cluster, cpu_maxfreq);
    write_sysfs("/proc/ppm/policy/hard_userlimit_max_cpu_freq", ppm_cmd);
    snprintf(ppm_cmd, sizeof(ppm_cmd), "%d %ld", data->cluster, min_target);
    write_sysfs("/proc/ppm/policy/hard_userlimit_min_cpu_freq", ppm_cmd);
    data->cluster++;
    return 1;
}

void cpufreq_ppm_max_perf() {
    struct ppm_policy_data data = {1, 0};
    iterate_sysfs_nodes("/sys/devices/system/cpu/cpufreq", "policy", ppm_policy_cb, &data);
}

void cpufreq_ppm_unlock() {
    struct ppm_policy_data data = {0, 0};
    iterate_sysfs_nodes("/sys/devices/system/cpu/cpufreq", "policy", ppm_policy_cb, &data);
}

static int cpufreq_policy_set_cb(const char *entry_path, const char *entry_name, void *userdata) {
    if (!is_policy_name(entry_name)) return 0;
    int mode = *((int *)userdata);
    long max_freq = read_policy_freq(entry_path, "cpuinfo_max_freq");
    long min_freq = read_policy_freq(entry_path, "cpuinfo_min_freq");
    if (max_freq <= 0) return 0;
    long min_target = min_freq > 0 ? min_freq : max_freq;
    if (mode == 1) min_target = LITE_MODE == 1 ? policy_available_freq(entry_path, 0, max_freq) : max_freq;
    char path[MAX_PATH_LEN];
    if (path_join(path, sizeof(path), entry_path, "scaling_max_freq")) write_ll(max_freq, path);
    if (path_join(path, sizeof(path), entry_path, "scaling_min_freq")) write_ll(min_target, path);
    return 1;
}

void cpufreq_max_perf() {
    int mode = 1;
    int policies = iterate_sysfs_nodes("/sys/devices/system/cpu/cpufreq", "policy", cpufreq_policy_set_cb, &mode);
    if (policies > 0) return;
    int cpu_count = get_cpu_count();
    for (int i = 0; i < cpu_count; i++) {
        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", i);
        long cpu_maxfreq = read_int_from_file(path);
        if (cpu_maxfreq <= 0) continue;
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
        write_ll(cpu_maxfreq, path);
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_available_frequencies", i);
        long min_target = LITE_MODE == 1 ? get_mid_freq(path) : cpu_maxfreq;
        if (min_target <= 0) min_target = cpu_maxfreq;
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
        write_ll(min_target, path);
    }
}

void cpufreq_unlock() {
    int mode = 0;
    int policies = iterate_sysfs_nodes("/sys/devices/system/cpu/cpufreq", "policy", cpufreq_policy_set_cb, &mode);
    if (policies > 0) return;
    int cpu_count = get_cpu_count();
    for (int i = 0; i < cpu_count; i++) {
        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", i);
        long cpu_maxfreq = read_int_from_file(path);
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_min_freq", i);
        long cpu_minfreq = read_int_from_file(path);
        if (cpu_maxfreq <= 0 || cpu_minfreq <= 0) continue;
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
        write_ll(cpu_maxfreq, path);
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
        write_ll(cpu_minfreq, path);
    }
}
