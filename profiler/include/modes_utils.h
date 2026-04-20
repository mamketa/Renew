/*
 * Copyright (C) 2025-2026 VelocityFox22
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "velfox_common.h"

/* Load all configuration from MODULE_CONFIG */
void read_configs(void);

/* DnD control (0 = off, 1 = priority) */
void set_dnd(int mode);

/* RPS/XPS tuning (mode: 1=perf, 2=balanced, 3=efficiency) */
void set_rps_xps(int mode);

/* Profile orchestration — ordered execution of all subsystems */
void perfcommon(void);
void esport_mode(void);
void balanced_mode(void);
void efficiency_mode(void);
