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

#include <velfox.h>

// Cached MLBB PID
pid_t mlbb_pid = 0;

/***********************************************************************************
 * Function Name      : handle_mlbb
 * Inputs             : const char *gamestart - Game package name
 * Returns            : MLBBState (enum)
 * Description        : Checks if Mobile Legends: Bang Bang IS actually running
 *                      on foreground, not in the background.
 ***********************************************************************************/
MLBBState handle_mlbb(const char* gamestart) {
    // Is Gamestart MLBB?
    if (!gamestart || !IS_MLBB(gamestart)) {
        mlbb_pid = 0;
        return MLBB_NOT_RUNNING;
    }

    /* Validate cached PID */
    if (mlbb_pid > 0) {
        errno = 0;
        if (kill(mlbb_pid, 0) == 0)
            return MLBB_RUNNING;
        if (errno == EPERM)
            return MLBB_RUNNING;

        mlbb_pid = 0;
    }

    /* Build process name */
    char mlbb_proc[64];
    int ret = snprintf(
        mlbb_proc,
        sizeof(mlbb_proc),
        "%s:UnityKillsMe",
        gamestart
    );

    if (ret < 0 || ret >= (int)sizeof(mlbb_proc)) {
        log_velfox(LOG_ERROR, "MLBB process name truncated");
        return MLBB_NOT_RUNNING;
    }

    /* Fetch PID */
    mlbb_pid = pidof(mlbb_proc);
    if (mlbb_pid > 0) {
        log_velfox(LOG_INFO,
                   "Boosting MLBB process %s", mlbb_proc);
        return MLBB_RUNNING;
    }
    return MLBB_RUN_BG;
}
