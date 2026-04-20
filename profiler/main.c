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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <mode>\n", argv[0]);
        printf("Modes:\n");
        printf("  0 - Common tweaks\n");
        printf("  1 - Esport mode\n");
        printf("  2 - Balanced mode\n");
        printf("  3 - Efficiency mode\n");
        return 1;
    }
    int mode = atoi(argv[1]);
    read_configs();
    switch (mode) {
        case 0: perfcommon(); break;
        case 1: perfcommon(); esport_mode(); break;
        case 2: perfcommon(); balanced_mode(); break;
        case 3: perfcommon(); efficiency_mode(); break;
        default:
            printf("Invalid mode: %d\n", mode);
            return 1;
    }
    return 0;
}
