/*
 * velfox_network.h — Network subsystem optimization API declarations
 *
 * Covers: TCP Congestion Control selection, TCP/IP parameter tuning,
 *         RPS (Receive Packet Steering) and XPS (Transmit Packet Steering)
 *         per-queue CPU affinity masks, and workqueue power-efficiency flag.
 *
 * All path literals used inside the implementation are resolved from
 * velfox_paths.h macros — never hard-coded inline.
 *
 * Standard: C23 / Clang
 */

#ifndef VELFOX_NETWORK_H
#define VELFOX_NETWORK_H

#include "velfox_common.h"

/* -------------------------------------------------------------------------
 * TCP baseline tuning (call once at startup via perfcommon)
 *
 * Selects the best available TCP congestion control algorithm from a
 * priority list: bbr3 > bbr2 > bbrplus > bbr > westwood > cubic.
 * Also enables SACK, ECN, window scaling, moderate receive buffer, and
 * TCP Fast Open (mode 3 = client + server).
 * ---------------------------------------------------------------------- */
void network_tune_tcp_baseline(void);

/* -------------------------------------------------------------------------
 * TCP latency profile
 *
 * Adjusts tcp_slow_start_after_idle based on the active performance mode.
 * Esport mode disables slow-start restart for sustained low-latency flows.
 *
 * @param mode  1 = esport (disabled), 2/3 = balanced / efficiency (enabled)
 * ---------------------------------------------------------------------- */
void network_tune_tcp_profile(int mode);

/* -------------------------------------------------------------------------
 * RPS / XPS CPU affinity mask tuning
 *
 * Iterates all network interfaces under /sys/class/net and writes an
 * appropriate CPU bitmask to each rx-* queue's rps_cpus and each tx-*
 * queue's xps_cpus.  The mask covers:
 *   mode 1 → all online CPUs  (esport: full spread)
 *   mode 2 → 75% of online CPUs  (balanced)
 *   mode 3 → 50% of online CPUs  (efficiency)
 *
 * Uses sysconf(_SC_NPROCESSORS_ONLN) to determine the live CPU count.
 * Entries whose sysfs paths exceed MAX_PATH_LEN are skipped safely.
 *
 * @param mode  1 = esport, 2 = balanced, 3 = efficiency
 * ---------------------------------------------------------------------- */
void network_tune_rps_xps(int mode);

/* -------------------------------------------------------------------------
 * Workqueue power-efficiency flag
 *
 * Controls /sys/module/workqueue/parameters/power_efficient.
 *   mode 1 → "N" (disabled — maximum throughput)
 *   mode 2 → "N" (disabled — slightly favors performance)
 *   mode 3 → "Y" (enabled  — trade throughput for power savings)
 *
 * @param mode  1 = esport, 2 = balanced, 3 = efficiency
 * ---------------------------------------------------------------------- */
void network_tune_workqueue(int mode);

#endif /* VELFOX_NETWORK_H */
