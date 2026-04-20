#ifndef VELFOX_CPUFREQ_H
#define VELFOX_CPUFREQ_H

#include "velfox_common.h"

int get_cpu_count(void);
void change_cpu_gov(const char *gov);
long get_max_freq(const char *path);
long get_min_freq(const char *path);
long get_mid_freq(const char *path);
void cpufreq_ppm_max_perf(void);
void cpufreq_max_perf(void);
void cpufreq_ppm_unlock(void);
void cpufreq_unlock(void);

#endif
