/*
 * velfox_modes.h — Performance profile conductor API declarations
 *
 * modes_utils.c acts as the central orchestrator for each performance profile.
 * It reads runtime configuration, invokes memory and network sub-modules, and
 * dispatches to SoC-specific routines.  No tuning logic lives here directly.
 *
 * Standard: C23 / Clang
 */

#ifndef VELFOX_MODES_H
#define VELFOX_MODES_H

#include "velfox_common.h"
#include "velfox_soc.h"

/*
 * read_configs() — Populate global runtime state from the module config files.
 * Must be called once before any mode function is invoked.
 */
void read_configs(void);

/*
 * perfcommon() — One-time baseline tweaks applied regardless of profile.
 * Disables panic-on-oops, applies TCP baseline, sets sched lib hints, etc.
 */
void perfcommon(void);

/*
 * esport_mode() — Maximum performance profile.
 * Optimizes for lowest possible input and render latency.
 */
void esport_mode(void);

/*
 * balanced_mode() — Default everyday performance profile.
 * Balances throughput, latency, and power draw.
 */
void balanced_mode(void);

/*
 * efficiency_mode() — Power-saving profile.
 * Reduces CPU/memory/network activity to extend battery life.
 */
void efficiency_mode(void);

/*
 * set_dnd() — Toggle Android Do-Not-Disturb via the notification command.
 * @param mode  0 = off, 1 = priority
 */
void set_dnd(int mode);

#endif /* VELFOX_MODES_H */
