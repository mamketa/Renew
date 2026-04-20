/*
 * velfox_paths.h — Centralized sysfs/procfs path definitions for Velfox Profiler
 *
 * All kernel node paths used across the project are defined here as macros.
 * Updating a path requires changes in exactly one place.
 *
 * Compilation target: Android rooted environment (ARM64)
 * Standard: C23 / Clang
 */

#ifndef VELFOX_PATHS_H
#define VELFOX_PATHS_H

/* -------------------------------------------------------------------------
 * Module configuration base
 * ---------------------------------------------------------------------- */
#define MODULE_CONFIG           "/data/adb/.config/Velfox"

/* -------------------------------------------------------------------------
 * Kernel panic / oops behavior
 * ---------------------------------------------------------------------- */
#define PATH_KERNEL_PANIC               "/proc/sys/kernel/panic"
#define PATH_KERNEL_PANIC_ON_OOPS       "/proc/sys/kernel/panic_on_oops"
#define PATH_KERNEL_PANIC_ON_WARN       "/proc/sys/kernel/panic_on_warn"
#define PATH_KERNEL_SOFTLOCKUP_PANIC    "/proc/sys/kernel/softlockup_panic"
#define PATH_KERNEL_PRINTK              "/proc/sys/kernel/printk"

/* -------------------------------------------------------------------------
 * Scheduler tuning
 * ---------------------------------------------------------------------- */
#define PATH_SCHED_ENERGY_AWARE              "/proc/sys/kernel/sched_energy_aware"
#define PATH_SCHED_BOOST                     "/proc/sys/kernel/sched_boost"
#define PATH_SCHED_NR_MIGRATE                "/proc/sys/kernel/sched_nr_migrate"
#define PATH_SCHED_RR_TIMESLICE_MS           "/proc/sys/kernel/sched_rr_timeslice_ms"
#define PATH_SCHED_TIME_AVG_MS               "/proc/sys/kernel/sched_time_avg_ms"
#define PATH_SCHED_TUNABLE_SCALING           "/proc/sys/kernel/sched_tunable_scaling"
#define PATH_SCHED_MIN_TASK_UTIL_BOOST       "/proc/sys/kernel/sched_min_task_util_for_boost"
#define PATH_SCHED_MIN_TASK_UTIL_COLOC       "/proc/sys/kernel/sched_min_task_util_for_colocation"
#define PATH_SCHED_MIGRATION_COST_NS         "/proc/sys/kernel/sched_migration_cost_ns"
#define PATH_SCHED_MIN_GRANULARITY_NS        "/proc/sys/kernel/sched_min_granularity_ns"
#define PATH_SCHED_WAKEUP_GRANULARITY_NS     "/proc/sys/kernel/sched_wakeup_granularity_ns"
#define PATH_SCHED_SCHEDSTATS                "/proc/sys/kernel/sched_schedstats"
#define PATH_SCHED_AUTOGROUP_ENABLED         "/proc/sys/kernel/sched_autogroup_enabled"
#define PATH_SCHED_CHILD_RUNS_FIRST          "/proc/sys/kernel/sched_child_runs_first"
#define PATH_SCHED_FEATURES                  "/sys/kernel/debug/sched_features"
#define PATH_PERF_CPU_TIME_MAX_PCT           "/proc/sys/kernel/perf_cpu_time_max_percent"
#define PATH_SPLIT_LOCK_MITIGATE             "/proc/sys/kernel/split_lock_mitigate"
#define PATH_TASK_CPUSTATS_ENABLE            "/proc/sys/kernel/task_cpustats_enable"
#define PATH_SCHED_LIB_NAME                  "/proc/sys/kernel/sched_lib_name"
#define PATH_SCHED_LIB_MASK_FORCE            "/proc/sys/kernel/sched_lib_mask_force"

/* -------------------------------------------------------------------------
 * Virtual Memory subsystem
 * ---------------------------------------------------------------------- */
#define PATH_VM_SWAPPINESS                   "/proc/sys/vm/swappiness"
#define PATH_VM_VFS_CACHE_PRESSURE           "/proc/sys/vm/vfs_cache_pressure"
#define PATH_VM_COMPACTION_PROACTIVENESS     "/proc/sys/vm/compaction_proactiveness"
#define PATH_VM_DIRTY_BG_RATIO               "/proc/sys/vm/dirty_background_ratio"
#define PATH_VM_DIRTY_RATIO                  "/proc/sys/vm/dirty_ratio"
#define PATH_VM_DIRTY_EXPIRE_CS              "/proc/sys/vm/dirty_expire_centisecs"
#define PATH_VM_DIRTY_WRITEBACK_CS           "/proc/sys/vm/dirty_writeback_centisecs"
#define PATH_VM_PAGE_CLUSTER                 "/proc/sys/vm/page-cluster"
#define PATH_VM_STAT_INTERVAL                "/proc/sys/vm/stat_interval"
#define PATH_VM_OVERCOMMIT_RATIO             "/proc/sys/vm/overcommit_ratio"
#define PATH_VM_EXTFRAG_THRESHOLD            "/proc/sys/vm/extfrag_threshold"
#define PATH_VM_WATERMARK_SCALE_FACTOR       "/proc/sys/vm/watermark_scale_factor"

/* -------------------------------------------------------------------------
 * MGLRU (Multi-Gen LRU)
 * ---------------------------------------------------------------------- */
#define PATH_MGLRU_ENABLED                   "/sys/kernel/mm/lru_gen/enabled"

/* -------------------------------------------------------------------------
 * cpuset / memory spread
 * ---------------------------------------------------------------------- */
#define PATH_CPUSET_BG_MEM_SPREAD            "/dev/cpuset/background/memory_spread_page"
#define PATH_CPUSET_SYSBG_MEM_SPREAD         "/dev/cpuset/system-background/memory_spread_page"
#define PATH_CPUSET_TOPAPP_MEM_SPREAD        "/dev/cpuset/top-app/memory_spread_page"
#define PATH_CPUSET_FGND_MEM_SPREAD          "/dev/cpuset/foreground/memory_spread_page"

/* -------------------------------------------------------------------------
 * uclamp — cpuctl cgroup paths (format: /dev/cpuctl/<group>/cpu.uclamp.{min,max})
 * These are used programmatically; base path defined here.
 * ---------------------------------------------------------------------- */
#define PATH_CPUCTL_BASE                     "/dev/cpuctl"
#define PATH_CGROUP_UCLAMP_BASE              "/sys/fs/cgroup/uclamp"

/* -------------------------------------------------------------------------
 * Block device base directories
 * ---------------------------------------------------------------------- */
#define PATH_SYSFS_BLOCK                     "/sys/block"
#define PATH_SYSFS_DEVFREQ                   "/sys/class/devfreq"
#define PATH_SYSFS_THERMAL                   "/sys/class/thermal"
#define PATH_SYSFS_NET                       "/sys/class/net"

/* -------------------------------------------------------------------------
 * I/O and block queue sub-paths (relative, appended to block device path)
 * ---------------------------------------------------------------------- */
#define LEAF_QUEUE_SCHEDULER                 "queue/scheduler"
#define LEAF_QUEUE_READ_AHEAD_KB             "queue/read_ahead_kb"
#define LEAF_QUEUE_NR_REQUESTS               "queue/nr_requests"
#define LEAF_QUEUE_IOSTATS                   "queue/iostats"
#define LEAF_QUEUE_ADD_RANDOM                "queue/add_random"

/* -------------------------------------------------------------------------
 * Network — TCP/IP tuning
 * ---------------------------------------------------------------------- */
#define PATH_NET_TCP_AVAILABLE_CC            "/proc/sys/net/ipv4/tcp_available_congestion_control"
#define PATH_NET_TCP_CONGESTION_CTRL         "/proc/sys/net/ipv4/tcp_congestion_control"
#define PATH_NET_TCP_SACK                    "/proc/sys/net/ipv4/tcp_sack"
#define PATH_NET_TCP_ECN                     "/proc/sys/net/ipv4/tcp_ecn"
#define PATH_NET_TCP_WINDOW_SCALING          "/proc/sys/net/ipv4/tcp_window_scaling"
#define PATH_NET_TCP_MODERATE_RCVBUF         "/proc/sys/net/ipv4/tcp_moderate_rcvbuf"
#define PATH_NET_TCP_FASTOPEN                "/proc/sys/net/ipv4/tcp_fastopen"
#define PATH_NET_TCP_SLOW_START_AFTER_IDLE   "/proc/sys/net/ipv4/tcp_slow_start_after_idle"

/* -------------------------------------------------------------------------
 * Network — RPS/XPS queue sub-paths
 * ---------------------------------------------------------------------- */
#define LEAF_RPS_CPUS                        "rps_cpus"
#define LEAF_XPS_CPUS                        "xps_cpus"

/* -------------------------------------------------------------------------
 * Workqueue
 * ---------------------------------------------------------------------- */
#define PATH_WQ_POWER_EFFICIENT              "/sys/module/workqueue/parameters/power_efficient"

/* -------------------------------------------------------------------------
 * CPU boost modules
 * ---------------------------------------------------------------------- */
#define PATH_CPU_BOOST_INPUT_FREQ            "/sys/module/cpu_boost/parameters/input_boost_freq"
#define PATH_CPU_BOOST_INPUT_MS              "/sys/module/cpu_boost/parameters/input_boost_ms"
#define PATH_CPU_INPUT_BOOST_FREQ            "/sys/module/cpu_input_boost/parameters/input_boost_freq"
#define PATH_CPU_INPUT_BOOST_MS              "/sys/module/cpu_input_boost/parameters/input_boost_ms"
#define PATH_KERNEL_CPU_INPUT_BOOST_FREQ     "/sys/kernel/cpu_input_boost/input_boost_freq"
#define PATH_KERNEL_CPU_INPUT_BOOST_MS       "/sys/kernel/cpu_input_boost/input_boost_ms"
#define PATH_MSM_PERF_TOUCHBOOST            "/sys/module/msm_performance/parameters/touchboost"

/* -------------------------------------------------------------------------
 * schedtune (legacy cgroup v1 stune)
 * ---------------------------------------------------------------------- */
#define PATH_STUNE_TOPAPP_PREFER_IDLE        "/dev/stune/top-app/schedtune.prefer_idle"
#define PATH_STUNE_TOPAPP_BOOST              "/dev/stune/top-app/schedtune.boost"

/* -------------------------------------------------------------------------
 * OEM-specific / vendor nodes
 * ---------------------------------------------------------------------- */
#define PATH_MMC_USE_SPI_CRC                 "/sys/module/mmc_core/parameters/use_spi_crc"
#define PATH_OPCHAIN_ENABLE                  "/sys/module/opchain/parameters/chain_on"
#define PATH_CPUFREQ_BOUNCING_ENABLE         "/sys/module/cpufreq_bouncing/parameters/enable"
#define PATH_TASK_SCHED_INFO_ENABLE          "/proc/task_info/task_sched_info/task_sched_info_enable"
#define PATH_OPLUS_SCHED_ASSIST              "/proc/oplus_scheduler/sched_assist/sched_assist_enabled"
#define PATH_PROC_PPM                        "/proc/ppm"

/* -------------------------------------------------------------------------
 * Touchpanel
 * ---------------------------------------------------------------------- */
#define PATH_TP_GAME_SWITCH                  "/proc/touchpanel/game_switch_enable"
#define PATH_TP_OPLUS_LIMIT_ENABLE           "/proc/touchpanel/oplus_tp_limit_enable"
#define PATH_TP_OPPO_LIMIT_ENABLE            "/proc/touchpanel/oppo_tp_limit_enable"
#define PATH_TP_OPLUS_DIRECTION              "/proc/touchpanel/oplus_tp_direction"
#define PATH_TP_OPPO_DIRECTION               "/proc/touchpanel/oppo_tp_direction"

/* -------------------------------------------------------------------------
 * Thermal zone policy sub-path (relative leaf)
 * ---------------------------------------------------------------------- */
#define LEAF_THERMAL_POLICY                  "policy"

#endif /* VELFOX_PATHS_H */
