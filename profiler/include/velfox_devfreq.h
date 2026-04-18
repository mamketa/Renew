#ifndef VELFOX_DEVFREQ_H
#define VELFOX_DEVFREQ_H

#include "velfox_common.h"

// Devfreq functions
int devfreq_max_perf(const char *path);
int devfreq_mid_perf(const char *path);
int devfreq_unlock(const char *path);
int devfreq_min_perf(const char *path);

// Qualcomm specific
int qcom_cpudcvs_max_perf(const char *path);
int qcom_cpudcvs_mid_perf(const char *path);
int qcom_cpudcvs_unlock(const char *path);
int qcom_cpudcvs_min_perf(const char *path);

// Mediatek specific
int mtk_gpufreq_minfreq_index(const char *path);
int mtk_gpufreq_midfreq_index(const char *path);

#endif // VELFOX_DEVFREQ_H