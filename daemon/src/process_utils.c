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

/***********************************************************************************
 * Function Name      : pidof
 * Inputs             : name (char *) - Name of process
 * Returns            : pid (pid_t) - PID of process
 * Description        : Fetch PID from a process name.
 * Note               : You can input inexact process name.
 ***********************************************************************************/
pid_t pidof(const char* name) {
    if (!name || !*name)
        return 0;

    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) [[clang::unlikely]] {
        perror("opendir");
        return 0;
    }

    pid_t tracked_pid = 0;
    struct dirent* entry;

    while ((entry = readdir(proc_dir)) != NULL) {

        // Validate numeric directory name
        const char* dname = entry->d_name;
        if (!isdigit((unsigned char)dname[0]))
            continue;

        bool is_pid = true;
        for (const char* p = dname; *p; ++p) {
            if (!isdigit((unsigned char)*p)) {
                is_pid = false;
                break;
            }
        }
        if (!is_pid)
            continue;

        // Convert PID once
        char* end;
        long pid_val = strtol(dname, &end, 10);
        if (end == dname || *end != '\0' || pid_val <= 0)
            continue;

        // Build cmdline path
        char path[64];
        if (snprintf(path, sizeof(path), "/proc/%ld/cmdline", pid_val) >= (int)sizeof(path))
            continue;

        FILE* fp = fopen(path, "r");
        if (!fp)
            continue;

        char cmdline[512];
        size_t len = fread(cmdline, 1, sizeof(cmdline) - 1, fp);
        fclose(fp);

        if (len == 0)
            continue;

        cmdline[len] = '\0';

        // Replace null separators with spaces
        for (size_t i = 0; i < len; ++i) {
            if (cmdline[i] == '\0')
                cmdline[i] = ' ';
        }

        // Substring match
        if (strstr(cmdline, name) != NULL) {
            if (tracked_pid == 0 || pid_val < tracked_pid)
                tracked_pid = (pid_t)pid_val;

            if (tracked_pid == 1)
                break;
        }
    }

    closedir(proc_dir);
    return tracked_pid;
}

/***********************************************************************************
 * Function Name      : uidof
 * Inputs             : pid (pid_t) - PID of process
 * Returns            : uid (int) - UID of process
 * Description        : Fetch UID from a process id.
 * Note               : Returns -1 on error.
 ***********************************************************************************/
int uidof(pid_t pid) {
    if (pid <= 0)
        return -1;

    char path[MAX_PATH_LENGTH];
    char line[MAX_DATA_LENGTH];
    FILE* status_file;
    int uid = -1;

    snprintf(path, sizeof(path), "/proc/%d/status", (int)pid);
    status_file = fopen(path, "r");
    if (!status_file)
        return -1;

    while (fgets(line, sizeof(line), status_file) != NULL) {
        if (strncmp(line, "Uid:", 4) == 0) {
            sscanf(line, "Uid:\t%d", &uid);
            break;
        }
    }

    fclose(status_file);
    return uid;
}

/***********************************************************************************
 * Function Name      : set_priority
 * Inputs             : pid (pid_t) - PID to be boosted
 * Returns            : None
 * Description        : Sets the maximum CPU nice priority and I/O priority of a
 *                      given process.
 ***********************************************************************************/
void set_priority(const pid_t pid) {
    if (pid <= 0)
        return;
    log_velfox(LOG_DEBUG, "Applying priority settings for PID %d", pid);
    
    if (setpriority(PRIO_PROCESS, pid, -15) == -1)
        log_velfox(LOG_ERROR, "Unable to set nice priority for %d", pid);

    if (syscall(SYS_ioprio_set, 1, pid, (2 << 13) | 0) == -1)
        log_velfox(LOG_ERROR, "Unable to set IO priority for %d", pid);
}
