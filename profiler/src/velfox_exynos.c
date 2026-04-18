#include "velfox_soc.h"
#include "velfox_cpufreq.h"
#include "velfox_devfreq.h"

static void tune_exynos_kernel_gpu(int mode) {
    const char *gpu_path = "/sys/kernel/gpu";
    char avail_path[MAX_PATH_LEN];
    snprintf(avail_path, sizeof(avail_path), "%s/gpu_available_frequencies", gpu_path);
    if (!file_exists(avail_path)) return;
    long max_freq = get_max_freq(avail_path);
    long min_freq = get_min_freq(avail_path);
    long mid_freq = get_mid_freq(avail_path);
    char max_clock_path[MAX_PATH_LEN];
    char min_clock_path[MAX_PATH_LEN];
    snprintf(max_clock_path, sizeof(max_clock_path), "%s/gpu_max_clock", gpu_path);
    snprintf(min_clock_path, sizeof(min_clock_path), "%s/gpu_min_clock", gpu_path);
    if (mode == 1) {
        write_ll(max_freq, max_clock_path);
        write_ll(LITE_MODE == 1 && mid_freq > 0 ? mid_freq : max_freq, min_clock_path);
    } else if (mode == 2) {
        write_ll(max_freq, max_clock_path);
        write_ll(min_freq, min_clock_path);
    } else {
        long target = mid_freq > 0 ? mid_freq : min_freq;
        write_ll(target, max_clock_path);
        write_ll(min_freq > 0 ? min_freq : target, min_clock_path);
    }
}

static int mali_policy_cb(const char *entry_path, const char *entry_name, void *userdata) {
    if (!strstr(entry_name, ".mali") && !strstr(entry_name, "mali")) return 0;
    int mode = *((int *)userdata);
    char path[MAX_PATH_LEN];
    if (!path_join(path, sizeof(path), entry_path, "power_policy")) return 0;
    if (mode == 1) return write_sysfs(path, "always_on");
    return write_sysfs(path, "coarse_demand");
}

static int exynos_mif_cb(const char *entry_path, const char *entry_name, void *userdata) {
    if (!strstr(entry_name, "devfreq_mif") && !strstr(entry_name, "mif")) return 0;
    int mode = *((int *)userdata);
    if (mode == 1) return LITE_MODE == 1 ? devfreq_mid_perf(entry_path) : devfreq_max_perf(entry_path);
    if (mode == 2) return devfreq_unlock(entry_path);
    return devfreq_mid_perf(entry_path);
}

static void tune_exynos_common(int mode) {
    tune_exynos_kernel_gpu(mode);
    iterate_sysfs_nodes("/sys/devices/platform", NULL, mali_policy_cb, &mode);
    if (DEVICE_MITIGATION == 0) iterate_sysfs_nodes("/sys/class/devfreq", NULL, exynos_mif_cb, &mode);
}

void exynos_esport() {
    tune_exynos_common(1);
}

void exynos_balanced() {
    tune_exynos_common(2);
}

void exynos_efficiency() {
    tune_exynos_common(3);
}
