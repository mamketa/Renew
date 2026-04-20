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

#include "soc_utils.h"
#include "velfox_common.h"

/*
 * soc_apply: dispatches the correct SOC-specific override for the given profile.
 * profile: 1=esport, 2=balanced, 3=efficiency
 * Uses the global SOC value set by read_configs().
 */
void soc_apply(int profile) {
    switch (SOC) {
        case SOC_MEDIATEK:
            if (profile == 1)       mediatek_esport();
            else if (profile == 2)  mediatek_balanced();
            else                    mediatek_efficiency();
            break;

        case SOC_SNAPDRAGON:
            if (profile == 1)       snapdragon_esport();
            else if (profile == 2)  snapdragon_balanced();
            else                    snapdragon_efficiency();
            break;

        case SOC_EXYNOS:
            if (profile == 1)       exynos_esport();
            else if (profile == 2)  exynos_balanced();
            else                    exynos_efficiency();
            break;

        case SOC_TENSOR:
            if (profile == 1)       tensor_esport();
            else if (profile == 2)  tensor_balanced();
            else                    tensor_efficiency();
            break;

        case SOC_UNISOC:
            if (profile == 1)       unisoc_esport();
            else if (profile == 2)  unisoc_balanced();
            else                    unisoc_efficiency();
            break;

        case SOC_GENERIC:
        default:
            /* Generic: no SOC-specific override needed */
            break;
    }
}
