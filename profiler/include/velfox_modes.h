#ifndef VELFOX_MODES_H
#define VELFOX_MODES_H

#include "velfox_common.h"
#include "velfox_soc.h"

void perfcommon();
void esport_mode();
void balanced_mode();
void efficiency_mode();
void set_rps_xps(int mode);
void set_dnd(int mode);
void read_configs();

#endif //VELFOX_MODES_H