#include "velfox_soc.h"
#include "velfox_cpufreq.h"
#include "velfox_devfreq.h"

static int unisoc_gpu_cb(const char *entry_path, const char *entry_name, void *userdata) {
    int mode = *((int *)userdata);
    if (!strstr(entry_name, ".gpu") && !strstr(entry_name, "gpu")) return 0;
    if (mode == 1) return LITE_MODE == 0 ? devfreq_max_perf(entry_path) : devfreq_mid_perf(entry_path);
    if (mode == 2) return devfreq_unlock(entry_path);
    return devfreq_mid_perf(entry_path);
}

static void tune_unisoc_gpu(int mode) {
    iterate_sysfs_nodes("/sys/class/devfreq", NULL, unisoc_gpu_cb, &mode);
}

void unisoc_esport() {
    tune_unisoc_gpu(1);
}

void unisoc_balanced() {
    tune_unisoc_gpu(2);
}

void unisoc_efficiency() {
    tune_unisoc_gpu(3);
}
