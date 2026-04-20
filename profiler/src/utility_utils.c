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

#include "utility_utils.h"

int node_exists(const char *path) {
    if (!path) return 0;
    return access(path, F_OK) == 0;
}

const char *probe_paths(const char *const *paths, int count) {
    for (int i = 0; i < count; i++) {
        if (paths[i] && access(paths[i], F_OK) == 0)
            return paths[i];
    }
    return NULL;
}

int read_int_from_file(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    int value = 0;
    if (fscanf(fp, "%d", &value) != 1)
        value = 0;
    fclose(fp);
    return value;
}

void read_string_from_file(char *buffer, size_t size, const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        if (buffer && size > 0) buffer[0] = '\0';
        return;
    }
    fgets(buffer, (int)size, fp);
    buffer[strcspn(buffer, "\n")] = '\0';
    fclose(fp);
}

/*
 * Core write logic shared by safe_write / apply / write_file.
 * - Silent skip if node missing
 * - Skip if value already matches (avoids redundant syscalls)
 * - chmod(0644) only if initial fopen("w") fails
 * - Restores chmod(0444) after write
 */
static int _do_write(const char *path, const char *value) {
    if (!path || !value) return 0;
    if (access(path, F_OK) != 0) return 0;

    char buf[256] = {0};
    FILE *fp = fopen(path, "r");
    if (fp) {
        if (fgets(buf, sizeof(buf), fp))
            buf[strcspn(buf, "\n")] = '\0';
        fclose(fp);
        if (strcmp(buf, value) == 0)
            return 1;
    }

    fp = fopen(path, "w");
    if (!fp) {
        chmod(path, 0644);
        fp = fopen(path, "w");
        if (!fp) return 0;
    }

    int ret = fprintf(fp, "%s", value);
    fflush(fp);
    fclose(fp);
    chmod(path, 0444);
    return (ret >= 0) ? 1 : 0;
}

int safe_write(const char *path, const char *value) {
    return _do_write(path, value);
}

int apply(const char *value, const char *path) {
    return _do_write(path, value);
}

int write_file(const char *value, const char *path) {
    return _do_write(path, value);
}

int apply_ll(long long value, const char *path) {
    char str[50];
    snprintf(str, sizeof(str), "%lld", value);
    return _do_write(path, str);
}

int write_ll(long long value, const char *path) {
    char str[50];
    snprintf(str, sizeof(str), "%lld", value);
    return _do_write(path, str);
}
