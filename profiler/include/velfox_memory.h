/*
 * velfox_memory.h — Memory subsystem tuning API declarations
 *
 * Covers: Virtual Memory parameters, MGLRU, uclamp cgroup profiles,
 *         input boost, I/O scheduler selection, block queue tuning,
 *         and modern devfreq (memory bus / interconnect) optimization.
 *
 * All path literals used inside the implementation are resolved from
 * velfox_paths.h macros — never hard-coded inline.
 *
 * Standard: C23 / Clang
 */

#ifndef VELFOX_MEMORY_H
#define VELFOX_MEMORY_H

#include "velfox_common.h"

/* -------------------------------------------------------------------------
 * Virtual Memory profile tuning
 *
 * Writes swappiness, vfs_cache_pressure, compaction_proactiveness,
 * dirty_* ratios and centisec timers to their /proc/sys/vm nodes.
 *
 * @param mode  1 = esport (low latency), 2 = balanced, 3 = efficiency
 * ---------------------------------------------------------------------- */
void memory_tune_vm_profile(int mode);

/* -------------------------------------------------------------------------
 * MGLRU enablement
 *
 * Unconditionally enables the Multi-Gen LRU reclaim algorithm via
 * /sys/kernel/mm/lru_gen/enabled.  No-op if the node is absent.
 * ---------------------------------------------------------------------- */
void memory_enable_mglru(void);

/* -------------------------------------------------------------------------
 * uclamp cgroup profile
 *
 * Writes cpu.uclamp.min / cpu.uclamp.max to both cpuctl (/dev/cpuctl)
 * and cgroup v2 uclamp (/sys/fs/cgroup/uclamp) hierarchies for the five
 * standard Android scheduling groups:
 *   top-app, foreground, background, system-background, restricted-background
 *
 * @param mode  1 = esport, 2 = balanced, 3 = efficiency
 * ---------------------------------------------------------------------- */
void memory_tune_uclamp_profile(int mode);

/* -------------------------------------------------------------------------
 * CPU input-boost parameters
 *
 * Writes input_boost_freq and input_boost_ms to all known boost module
 * nodes (cpu_boost, cpu_input_boost, msm_performance).  Missing nodes are
 * silently skipped via the write_sysfs() existence check.
 *
 * @param mode  1 = esport (high boost), 2 = balanced, 3 = efficiency (low)
 * ---------------------------------------------------------------------- */
void memory_tune_input_boost(int mode);

/* -------------------------------------------------------------------------
 * I/O scheduler selection
 *
 * Iterates /sys/block, skipping loop and ram devices, and sets the queue
 * scheduler to the best available option from a priority list:
 *   esport     → deadline > zen
 *   balanced   → cfq
 *   efficiency → noop
 *
 * Uses sysfs_value_supported() to confirm availability before writing.
 *
 * @param mode  1 = esport, 2 = balanced, 3 = efficiency
 * ---------------------------------------------------------------------- */
void memory_set_io_scheduler(int mode);

/* -------------------------------------------------------------------------
 * Block queue parameter tuning
 *
 * Writes read_ahead_kb and nr_requests for each real block device.
 * Loop and ram devices are excluded from tuning.
 *
 * @param mode  1 = esport (32 KB / 32 req), 2 = balanced (128 KB / 64 req),
 *              3 = efficiency (64 KB / 32 req)
 * ---------------------------------------------------------------------- */
void memory_tune_block_queues(int mode);

/* -------------------------------------------------------------------------
 * Modern devfreq (memory bus / interconnect) tuning
 *
 * Iterates /sys/class/devfreq and tunes entries whose names match known
 * memory-interconnect patterns (cpu-lat, cpubw, llcc, l3, memlat, etc.).
 * Respects DEVICE_MITIGATION global: skips tuning entirely if non-zero.
 *
 * @param mode  1 = esport (max/mid perf depending on LITE_MODE),
 *              2 = balanced (unlock), 3 = efficiency (mid perf)
 * ---------------------------------------------------------------------- */
void memory_tune_devfreq(int mode);

#endif /* VELFOX_MEMORY_H */
