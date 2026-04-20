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

/* SOC identifiers (must match config values) */
#define SOC_GENERIC     0
#define SOC_MEDIATEK    1
#define SOC_SNAPDRAGON  2
#define SOC_EXYNOS      3
#define SOC_TENSOR      4
#define SOC_UNISOC      5

/* MediaTek */
void mediatek_esport(void);
void mediatek_balanced(void);
void mediatek_efficiency(void);

/* Snapdragon */
void snapdragon_esport(void);
void snapdragon_balanced(void);
void snapdragon_efficiency(void);

/* Exynos */
void exynos_esport(void);
void exynos_balanced(void);
void exynos_efficiency(void);

/* Tensor (Google) */
void tensor_esport(void);
void tensor_balanced(void);
void tensor_efficiency(void);

/* Unisoc */
void unisoc_esport(void);
void unisoc_balanced(void);
void unisoc_efficiency(void);

/* Dispatch SOC override for the given profile (1=esport, 2=balanced, 3=efficiency) */
void soc_apply(int profile);
