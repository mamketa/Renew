#ifndef VELFOX_MISC_H
#define VELFOX_MISC_H

#include "velfox_common.h"

void set_dnd(int mode);
void set_rps_xps(int mode);
void misc_common_tweaks(void);
void misc_mode_sched_tweaks(int mode);
void misc_mode_io_tweaks(int mode);
void misc_mode_vm_tweaks(int mode);
void misc_storage_devfreq_tweaks(int mode);
void misc_universal_tweaks(int mode);

#endif
