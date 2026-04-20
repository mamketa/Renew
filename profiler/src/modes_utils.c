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

#include "modes_utils.h"
#include "cpu_utils.h"
#include "devfreq_utils.h"
#include "memory_utils.h"
#include "io_utils.h"
#include "scheduler_utils.h"
#include "soc_utils.h"
#include "utility_utils.h"
#include "velfox_common.h"

/* Global variable definitions */
int  SOC              = 0;
int  LITE_MODE        = 0;
int  DEVICE_MITIGATION = 0;
char DEFAULT_CPU_GOV[50] = "schedutil";
char PPM_POLICY[512]  = "";

void read_configs(void) {
    char path[MAX_PATH_LEN];

    snprintf(path, sizeof(path), "%s/soc_recognition", MODULE_CONFIG);
    SOC = read_int_from_file(path);

    snprintf(path, sizeof(path), "%s/lite_mode", MODULE_CONFIG);
    LITE_MODE = read_int_from_file(path);

    snprintf(path, sizeof(path), "%s/ppm_policies_mediatek", MODULE_CONFIG);
    read_string_from_file(PPM_POLICY, sizeof(PPM_POLICY), path);

    snprintf(path, sizeof(path), "%s/custom_default_cpu_gov", MODULE_CONFIG);
    if (node_exists(path)) {
        read_string_from_file(DEFAULT_CPU_GOV, sizeof(DEFAULT_CPU_GOV), path);
    } else {
        snprintf(path, sizeof(path), "%s/default_cpu_gov", MODULE_CONFIG);
        read_string_from_file(DEFAULT_CPU_GOV, sizeof(DEFAULT_CPU_GOV), path);
    }

    snprintf(path, sizeof(path), "%s/device_mitigation", MODULE_CONFIG);
    DEVICE_MITIGATION = read_int_from_file(path);
}

void set_dnd(int mode) {
    if (mode == 0)
        system("cmd notification set_dnd off");
    else if (mode == 1)
        system("cmd notification set_dnd priority");
}

void set_rps_xps(int mode) {
    int cores = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (cores <= 0) return;

    int use = cores;
    if (mode == 1)       use = cores;
    else if (mode == 2)  use = (cores * 3) / 4;
    else if (mode == 3)  use = cores / 2;

    if (use < 1) use = 1;
    if (use > (int)(sizeof(unsigned long) * 8))
        use = (int)(sizeof(unsigned long) * 8);

    unsigned long mask = 0;
    for (int i = 0; i < use; i++)
        mask |= (1UL << i);

    char mask_hex[32];
    snprintf(mask_hex, sizeof(mask_hex), "%lx", mask);

    DIR *netdir = opendir("/sys/class/net");
    if (!netdir) return;
    struct dirent *iface;
    while ((iface = readdir(netdir)) != NULL) {
        if (!iface->d_name[0] || iface->d_name[0] == '.') continue;

        char queue_dir[MAX_PATH_LEN];
        if (snprintf(queue_dir, sizeof(queue_dir),
                     "/sys/class/net/%s/queues", iface->d_name)
            >= (int)sizeof(queue_dir))
            continue;

        DIR *qdir = opendir(queue_dir);
        if (!qdir) continue;
        struct dirent *queue;
        while ((queue = readdir(qdir)) != NULL) {
            if (!queue->d_name[0] || queue->d_name[0] == '.') continue;
            char path[MAX_PATH_LEN];
            /* RX = RPS */
            if (strncmp(queue->d_name, "rx-", 3) == 0) {
                snprintf(path, sizeof(path), "%s/%s/rps_cpus",
                         queue_dir, queue->d_name);
                apply(mask_hex, path);
            }
            /* TX = XPS */
            else if (strncmp(queue->d_name, "tx-", 3) == 0) {
                snprintf(path, sizeof(path), "%s/%s/xps_cpus",
                         queue_dir, queue->d_name);
                apply(mask_hex, path);
            }
        }
        closedir(qdir);
    }
    closedir(netdir);
}

/*
 * perfcommon — baseline tweaks shared by all profiles.
 *
 * Execution order:
 *   1. kernel safety params
 *   2. I/O common baseline
 *   3. scheduler common baseline
 *   4. VM common baseline
 *   5. thermal governor
 *   6. vendor-specific disables
 */
void perfcommon(void) {
    /* Disable kernel panic / oops escalation */
    apply("0", "/proc/sys/kernel/panic");
    apply("0", "/proc/sys/kernel/panic_on_oops");
    apply("0", "/proc/sys/kernel/panic_on_warn");
    apply("0", "/proc/sys/kernel/softlockup_panic");

    /* Sync pending writes to storage */
    (void)system("sync");

    /* I/O common baseline */
    io_common();

    /* Scheduler common baseline */
    sched_common();

    /* TCP tuning */
    {
        static const char *algs[] = {
            "bbr3", "bbr2", "bbrplus", "bbr", "westwood", "cubic"
        };
        for (int i = 0; i < 6; i++) {
            char cmd[MAX_PATH_LEN];
            snprintf(cmd, sizeof(cmd),
                     "grep -q \"%s\" /proc/sys/net/ipv4/tcp_available_congestion_control",
                     algs[i]);
            if (system(cmd) == 0) {
                apply(algs[i], "/proc/sys/net/ipv4/tcp_congestion_control");
                break;
            }
        }
        apply("1", "/proc/sys/net/ipv4/tcp_sack");
        apply("1", "/proc/sys/net/ipv4/tcp_ecn");
        apply("1", "/proc/sys/net/ipv4/tcp_window_scaling");
        apply("1", "/proc/sys/net/ipv4/tcp_moderate_rcvbuf");
        apply("3", "/proc/sys/net/ipv4/tcp_fastopen");
    }

    /* Reduce kernel log noise */
    apply("0 0 0 0", "/proc/sys/kernel/printk");

    /* Disable SPI CRC */
    apply("0", "/sys/module/mmc_core/parameters/use_spi_crc");

    /* Disable OnePlus opchain */
    apply("0", "/sys/module/opchain/parameters/chain_on");

    /* VM common baseline (page-cluster, stat_interval, etc.) */
    apply("0",   "/proc/sys/vm/page-cluster");
    apply("15",  "/proc/sys/vm/stat_interval");
    apply("80",  "/proc/sys/vm/overcommit_ratio");
    apply("640", "/proc/sys/vm/extfrag_threshold");
    apply("22",  "/proc/sys/vm/watermark_scale_factor");
    apply("0",   "/dev/cpuset/background/memory_spread_page");
    apply("0",   "/dev/cpuset/system-background/memory_spread_page");

    /* Thermal governor: step_wise on all zones */
    DIR *tdir = opendir("/sys/class/thermal");
    if (tdir) {
        struct dirent *ent;
        while ((ent = readdir(tdir)) != NULL) {
            if (!strstr(ent->d_name, "thermal_zone")) continue;
            char path[MAX_PATH_LEN];
            snprintf(path, sizeof(path),
                     "/sys/class/thermal/%s/policy", ent->d_name);
            apply("step_wise", path);
        }
        closedir(tdir);
    }
}

/*
 * esport_mode — execution order:
 *   1. CPU
 *   2. GPU
 *   3. DEVFREQ / BUS
 *   4. MEMORY
 *   5. I/O
 *   6. SCHEDULER
 *   7. SOC overrides
 *   8. finalization (DnD, power efficient workqueue)
 */
void esport_mode(void) {
    /* CPU */
    if (LITE_MODE == 0 && DEVICE_MITIGATION == 0)
        change_cpu_gov("performance");
    else
        change_cpu_gov(DEFAULT_CPU_GOV);

    if (node_exists("/proc/ppm"))
        cpufreq_ppm_max_perf();
    else
        cpufreq_max_perf();

    /* eMMC/UFS devfreq */
    {
        DIR *dir = opendir("/sys/class/devfreq");
        if (dir) {
            struct dirent *ent;
            while ((ent = readdir(dir)) != NULL) {
                if (!strstr(ent->d_name, ".ufshc") &&
                    !strstr(ent->d_name, "mmc")) continue;
                char path[MAX_PATH_LEN];
                snprintf(path, sizeof(path), "/sys/class/devfreq/%s", ent->d_name);
                if (LITE_MODE == 1) devfreq_mid_perf(path);
                else                devfreq_max_perf(path);
            }
            closedir(dir);
        }
    }

    /* MEMORY */
    memory_tune_esport();

    /* I/O */
    io_tune_esport();

    /* SCHEDULER */
    sched_tune_esport();

    /* SOC override */
    soc_apply(1);

    /* Disable battery saver workqueue */
    apply("N", "/sys/module/workqueue/parameters/power_efficient");

    /* RPS/XPS: all cores */
    set_rps_xps(1);

    /* TCP: disable slow start after idle */
    apply("0", "/proc/sys/net/ipv4/tcp_slow_start_after_idle");

    /* DnD */
    char dnd_path[MAX_PATH_LEN];
    snprintf(dnd_path, sizeof(dnd_path), "%s/dnd_gameplay", MODULE_CONFIG);
    if (read_int_from_file(dnd_path) == 1)
        set_dnd(1);

    /* Touchpanel gaming mode (Oppo/Oplus/Realme) */
    apply("1", "/proc/touchpanel/game_switch_enable");
    apply("0", "/proc/touchpanel/oplus_tp_limit_enable");
    apply("0", "/proc/touchpanel/oppo_tp_limit_enable");
    apply("1", "/proc/touchpanel/oplus_tp_direction");
    apply("1", "/proc/touchpanel/oppo_tp_direction");
}

/*
 * balanced_mode — execution order mirrors esport_mode.
 */
void balanced_mode(void) {
    /* CPU: default governor, unlock */
    change_cpu_gov(DEFAULT_CPU_GOV);
    if (node_exists("/proc/ppm"))
        cpufreq_ppm_unlock();
    else
        cpufreq_unlock();

    /* eMMC/UFS devfreq: unlock */
    {
        DIR *dir = opendir("/sys/class/devfreq");
        if (dir) {
            struct dirent *ent;
            while ((ent = readdir(dir)) != NULL) {
                if (!strstr(ent->d_name, ".ufshc") &&
                    !strstr(ent->d_name, "mmc")) continue;
                char path[MAX_PATH_LEN];
                snprintf(path, sizeof(path), "/sys/class/devfreq/%s", ent->d_name);
                devfreq_unlock(path);
            }
            closedir(dir);
        }
    }

    /* MEMORY */
    memory_tune_balanced();

    /* I/O */
    io_tune_balanced();

    /* SCHEDULER */
    sched_tune_balanced();

    /* SOC override */
    soc_apply(2);

    /* Restore battery saver */
    apply("Y", "/sys/module/workqueue/parameters/power_efficient");

    /* RPS/XPS: 3/4 of cores */
    set_rps_xps(2);

    /* Restore DnD off */
    set_dnd(0);
}

/*
 * efficiency_mode — CPU, GPU, BUS all reduced evenly. No spikes.
 */
void efficiency_mode(void) {
    /* CPU: schedutil, unlock natural range */
    change_cpu_gov(DEFAULT_CPU_GOV);
    if (node_exists("/proc/ppm"))
        cpufreq_ppm_unlock();
    else
        cpufreq_unlock();

    /* eMMC/UFS devfreq: minimum */
    {
        DIR *dir = opendir("/sys/class/devfreq");
        if (dir) {
            struct dirent *ent;
            while ((ent = readdir(dir)) != NULL) {
                if (!strstr(ent->d_name, ".ufshc") &&
                    !strstr(ent->d_name, "mmc")) continue;
                char path[MAX_PATH_LEN];
                snprintf(path, sizeof(path), "/sys/class/devfreq/%s", ent->d_name);
                devfreq_min_perf(path);
            }
            closedir(dir);
        }
    }

    /* MEMORY */
    memory_tune_efficiency();

    /* I/O */
    io_tune_efficiency();

    /* SCHEDULER */
    sched_tune_efficiency();

    /* SOC override */
    soc_apply(3);

    /* Re-enable battery saver */
    apply("Y", "/sys/module/workqueue/parameters/power_efficient");

    /* RPS/XPS: half cores */
    set_rps_xps(3);

    /* Restore DnD off */
    set_dnd(0);
}
