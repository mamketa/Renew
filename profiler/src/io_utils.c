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

#include "io_utils.h"
#include "utility_utils.h"
#include "velfox_common.h"

/*
 * _set_scheduler_on_block:
 * Attempts schedulers in priority order on a given block device queue path.
 * Auto-detects what the kernel actually accepts.
 */
static void _set_scheduler_on_block(const char *blk_name, const char *const *scheds, int count) {
    char sched_path[MAX_PATH_LEN];
    snprintf(sched_path, sizeof(sched_path),
             "/sys/block/%s/queue/scheduler", blk_name);
    if (!node_exists(sched_path)) return;

    char avail[512] = {0};
    FILE *fp = fopen(sched_path, "r");
    if (!fp) return;
    fgets(avail, sizeof(avail), fp);
    fclose(fp);

    for (int i = 0; i < count; i++) {
        if (strstr(avail, scheds[i])) {
            apply(scheds[i], sched_path);
            return;
        }
    }
}

/* Common baseline: disable iostats, add_random for all block devices */
void io_common(void) {
    DIR *dir = opendir("/sys/block");
    if (!dir) return;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "/sys/block/%s/queue/iostats", ent->d_name);
        apply("0", path);
        snprintf(path, sizeof(path), "/sys/block/%s/queue/add_random", ent->d_name);
        apply("0", path);
    }
    closedir(dir);
}

/*
 * Esport I/O:
 * Scheduler: deadline
 * Low read_ahead (32 KB), moderate nr_requests
 */
void io_tune_esport(void) {
    static const char *scheds[] = {"deadline", "kyber", "zen", "bfq", "noop"};
    static const char *blks[]   = {"mmcblk0", "mmcblk1", "sda", "sdb"};

    DIR *dir = opendir("/sys/block");
    if (!dir) return;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        _set_scheduler_on_block(ent->d_name, scheds, 3);

        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "/sys/block/%s/queue/read_ahead_kb", ent->d_name);
        apply("32", path);
        snprintf(path, sizeof(path), "/sys/block/%s/queue/nr_requests", ent->d_name);
        apply("32", path);
    }
    closedir(dir);
    (void)blks;
}

/*
 * Balanced I/O:
 * Scheduler: cfq → all fallback
 * Moderate read_ahead (128 KB)
 */
void io_tune_balanced(void) {
    static const char *scheds[] = {"bfq", "cfq", "kyber", "deadline", "noop"};

    DIR *dir = opendir("/sys/block");
    if (!dir) return;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        _set_scheduler_on_block(ent->d_name, scheds, 3);

        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "/sys/block/%s/queue/read_ahead_kb", ent->d_name);
        apply("128", path);
        snprintf(path, sizeof(path), "/sys/block/%s/queue/nr_requests", ent->d_name);
        apply("64", path);
    }
    closedir(dir);
}

/*
 * Efficiency I/O:
 * Scheduler: noop → all fallback
 * Higher read_ahead to prefetch more with fewer interrupts
 */
void io_tune_efficiency(void) {
    static const char *scheds[] = {"noop", "none", "kyber", "deadline"};

    DIR *dir = opendir("/sys/block");
    if (!dir) return;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        _set_scheduler_on_block(ent->d_name, scheds, 3);

        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "/sys/block/%s/queue/read_ahead_kb", ent->d_name);
        apply("256", path);
        snprintf(path, sizeof(path), "/sys/block/%s/queue/nr_requests", ent->d_name);
        apply("128", path);
    }
    closedir(dir);
}
