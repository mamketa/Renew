#ifndef VELFOX_CPUFREQ_H
#define VELFOX_CPUFREQ_H

#include "velfox_common.h"

/* Auto detect CPU count*/
int get_cpu_count(void);

// CPU Governor functions
void change_cpu_gov(const char *gov);

// Frequency calculation functions
long get_max_freq(const char *path);
long get_min_freq(const char *path);
long get_mid_freq(const char *path);

// CPU frequency setting functions
void cpufreq_ppm_max_perf();
void cpufreq_max_perf();
void cpufreq_ppm_unlock();
void cpufreq_unlock();

#endif //VELFOX_CPUFREQ_H