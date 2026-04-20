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

#include "memory_utils.h"
#include "utility_utils.h"

/* Common VM baseline applied by perfcommon() */
static void _vm_common(void) {
    apply("0",   "/proc/sys/vm/page-cluster");
    apply("15",  "/proc/sys/vm/stat_interval");
    apply("80",  "/proc/sys/vm/overcommit_ratio");
    apply("640", "/proc/sys/vm/extfrag_threshold");
    apply("22",  "/proc/sys/vm/watermark_scale_factor");

    apply("0", "/dev/cpuset/background/memory_spread_page");
    apply("0", "/dev/cpuset/system-background/memory_spread_page");
}

void memory_tune_esport(void) {
    _vm_common();
    /* Esport: minimize latency — prefer memory in active use over reclaim */
    apply("10",  "/proc/sys/vm/swappiness");
    apply("100", "/proc/sys/vm/vfs_cache_pressure");
    apply("3",   "/proc/sys/vm/drop_caches");
}

void memory_tune_balanced(void) {
    _vm_common();
    apply("60",  "/proc/sys/vm/swappiness");
    apply("100", "/proc/sys/vm/vfs_cache_pressure");
}

void memory_tune_efficiency(void) {
    _vm_common();
    /* Efficiency: higher swappiness to reclaim pages early, reducing memory pressure */
    apply("80",  "/proc/sys/vm/swappiness");
    apply("200", "/proc/sys/vm/vfs_cache_pressure");
}
