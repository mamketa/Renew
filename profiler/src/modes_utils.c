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
 
#include "velfox_modes.h"
#include "velfox_cpufreq.h"
#include "velfox_misc.h"
#include "velfox_soc.h"

int SOC = 0;
int LITE_MODE = 0;
int DEVICE_MITIGATION = 0;
char DEFAULT_CPU_GOV[50] = "schedutil";
char PPM_POLICY[512] = "";

void read_configs(void) {
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s/soc_recognition", MODULE_CONFIG);
    SOC = read_int_from_file(path);
    snprintf(path, sizeof(path), "%s/lite_mode", MODULE_CONFIG);
    LITE_MODE = read_int_from_file(path);
    snprintf(path, sizeof(path), "%s/ppm_policies_mediatek", MODULE_CONFIG);
    read_string_from_file(PPM_POLICY, sizeof(PPM_POLICY), path);
    snprintf(path, sizeof(path), "%s/custom_default_cpu_gov", MODULE_CONFIG);
    if (file_exists(path)) read_string_from_file(DEFAULT_CPU_GOV, sizeof(DEFAULT_CPU_GOV), path);
    else {
        snprintf(path, sizeof(path), "%s/default_cpu_gov", MODULE_CONFIG);
        read_string_from_file(DEFAULT_CPU_GOV, sizeof(DEFAULT_CPU_GOV), path);
    }
    snprintf(path, sizeof(path), "%s/device_mitigation", MODULE_CONFIG);
    DEVICE_MITIGATION = read_int_from_file(path);
    if (!DEFAULT_CPU_GOV[0]) snprintf(DEFAULT_CPU_GOV, sizeof(DEFAULT_CPU_GOV), "schedutil");
}

void perfcommon(void) {
    misc_common_tweaks();
}

static void touchpanel_tweaks(int mode) {
    if (mode == 1) {
        apply("1", "/proc/touchpanel/game_switch_enable");
        apply("0", "/proc/touchpanel/oplus_tp_limit_enable");
        apply("0", "/proc/touchpanel/oppo_tp_limit_enable");
        apply("1", "/proc/touchpanel/oplus_tp_direction");
        apply("1", "/proc/touchpanel/oppo_tp_direction");
    } else {
        apply("0", "/proc/touchpanel/game_switch_enable");
        apply("1", "/proc/touchpanel/oplus_tp_limit_enable");
        apply("1", "/proc/touchpanel/oppo_tp_limit_enable");
        apply("0", "/proc/touchpanel/oplus_tp_direction");
        apply("0", "/proc/touchpanel/oppo_tp_direction");
    }
}

static void dispatch_soc(int mode) {
    switch (SOC) {
        case 1: if (mode == 1) mediatek_esport(); else if (mode == 2) mediatek_balanced(); else mediatek_efficiency(); break;
        case 2: if (mode == 1) snapdragon_esport(); else if (mode == 2) snapdragon_balanced(); else snapdragon_efficiency(); break;
        case 3: if (mode == 1) exynos_esport(); else if (mode == 2) exynos_balanced(); else exynos_efficiency(); break;
        case 4: if (mode == 1) unisoc_esport(); else if (mode == 2) unisoc_balanced(); else unisoc_efficiency(); break;
        case 5: if (mode == 1) tensor_esport(); else if (mode == 2) tensor_balanced(); else tensor_efficiency(); break;
        default: break;
    }
}

static void run_mode(int mode) {
    char dnd_path[MAX_PATH_LEN];
    snprintf(dnd_path, sizeof(dnd_path), "%s/dnd_gameplay", MODULE_CONFIG);
    if (read_int_from_file(dnd_path) == 1) set_dnd(mode == 1 ? 1 : 0);
    apply(mode == 3 ? "Y" : "N", "/sys/module/workqueue/parameters/power_efficient");
    set_rps_xps(mode);
    misc_mode_sched_tweaks(mode);
    misc_universal_tweaks(mode);
    touchpanel_tweaks(mode);
    misc_storage_devfreq_tweaks(mode);
    if (mode == 1) {
        if (LITE_MODE == 0 && DEVICE_MITIGATION == 0) change_cpu_gov("performance");
        else change_cpu_gov(DEFAULT_CPU_GOV);
        if (file_exists("/proc/ppm")) cpufreq_ppm_max_perf();
        else cpufreq_max_perf();
    } else if (mode == 2) {
        change_cpu_gov(DEFAULT_CPU_GOV);
        if (file_exists("/proc/ppm")) cpufreq_ppm_unlock();
        else cpufreq_unlock();
    } else {
        char efficiency_gov_path[MAX_PATH_LEN];
        char efficiency_gov[50];
        snprintf(efficiency_gov_path, sizeof(efficiency_gov_path), "%s/efficiency_cpu_gov", MODULE_CONFIG);
        read_string_from_file(efficiency_gov, sizeof(efficiency_gov), efficiency_gov_path);
        change_cpu_gov(efficiency_gov[0] ? efficiency_gov : DEFAULT_CPU_GOV);
    }
    misc_mode_io_tweaks(mode);
    misc_mode_vm_tweaks(mode);
    dispatch_soc(mode);
}

void esport_mode(void) { run_mode(1); }
void balanced_mode(void) { run_mode(2); }
void efficiency_mode(void) { run_mode(3); }
