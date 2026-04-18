#include "velfox_soc.h"
#include "velfox_cpufreq.h"
#include "velfox_devfreq.h"

static int tensor_mali_cb(const char *entry_path, const char *entry_name, void *userdata) {
    if (!strstr(entry_name, ".mali") && !strstr(entry_name, "mali")) return 0;
    int mode = *((int *)userdata);
    char avail_path[MAX_PATH_LEN];
    if (!path_join(avail_path, sizeof(avail_path), entry_path, "available_frequencies")) return 0;
    if (!file_exists(avail_path)) return 0;
    long max_freq = get_max_freq(avail_path);
    long min_freq = get_min_freq(avail_path);
    long mid_freq = get_mid_freq(avail_path);
    char max_freq_path[MAX_PATH_LEN];
    char min_freq_path[MAX_PATH_LEN];
    path_join(max_freq_path, sizeof(max_freq_path), entry_path, "scaling_max_freq");
    path_join(min_freq_path, sizeof(min_freq_path), entry_path, "scaling_min_freq");
    if (mode == 1) {
        write_ll(max_freq, max_freq_path);
        write_ll(LITE_MODE == 1 && mid_freq > 0 ? mid_freq : max_freq, min_freq_path);
    } else if (mode == 2) {
        write_ll(max_freq, max_freq_path);
        write_ll(min_freq, min_freq_path);
    } else {
        long target = mid_freq > 0 ? mid_freq : min_freq;
        write_ll(target, max_freq_path);
        write_ll(min_freq > 0 ? min_freq : target, min_freq_path);
    }
    return 1;
}

static int tensor_mif_cb(const char *entry_path, const char *entry_name, void *userdata) {
    if (!strstr(entry_name, "devfreq_mif") && !strstr(entry_name, "mif")) return 0;
    int mode = *((int *)userdata);
    if (mode == 1) return LITE_MODE == 1 ? devfreq_mid_perf(entry_path) : devfreq_max_perf(entry_path);
    if (mode == 2) return devfreq_unlock(entry_path);
    return devfreq_mid_perf(entry_path);
}

static void tune_tensor_common(int mode) {
    iterate_sysfs_nodes("/sys/devices/platform", NULL, tensor_mali_cb, &mode);
    if (DEVICE_MITIGATION == 0) iterate_sysfs_nodes("/sys/class/devfreq", NULL, tensor_mif_cb, &mode);
}

void tensor_esport() {
    tune_tensor_common(1);
}

void tensor_balanced() {
    tune_tensor_common(2);
}

void tensor_efficiency() {
    tune_tensor_common(3);
}
