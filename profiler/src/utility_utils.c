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
 
#include "velfox_common.h"

int file_exists(const char *path) {
    return path && access(path, F_OK) == 0;
}

int read_int_from_file(const char *path) {
    FILE *fp = path ? fopen(path, "r") : NULL;
    if (!fp) return 0;
    int value = 0;
    if (fscanf(fp, "%d", &value) != 1) value = 0;
    fclose(fp);
    return value;
}

void read_string_from_file(char *buffer, size_t size, const char *path) {
    if (!buffer || size == 0) return;
    FILE *fp = path ? fopen(path, "r") : NULL;
    if (!fp) {
        buffer[0] = '\0';
        return;
    }
    if (!fgets(buffer, size, fp)) buffer[0] = '\0';
    buffer[strcspn(buffer, "\n")] = '\0';
    fclose(fp);
}

static int mode_is_writable(mode_t mode) {
    return (mode & 0222) != 0;
}

static int write_value(const char *value, const char *path, int restore_readonly) {
    if (!path || !value || access(path, F_OK) != 0) return 0;
    char current[256] = {0};
    FILE *fp = fopen(path, "r");
    if (fp) {
        if (fgets(current, sizeof(current), fp)) {
            current[strcspn(current, "\n")] = '\0';
            if (strcmp(current, value) == 0) {
                fclose(fp);
                return 1;
            }
        }
        fclose(fp);
    }
    struct stat st;
    int have_stat = stat(path, &st) == 0;
    mode_t old_mode = have_stat ? (st.st_mode & 0777) : 0;
    fp = fopen(path, "w");
    if (!fp && (!have_stat || !mode_is_writable(old_mode))) {
        chmod(path, 0644);
        fp = fopen(path, "w");
    }
    if (!fp) return 0;
    int ret = fprintf(fp, "%s", value);
    fflush(fp);
    fclose(fp);
    if (restore_readonly) chmod(path, 0444);
    else if (have_stat && !mode_is_writable(old_mode)) chmod(path, old_mode);
    return ret >= 0;
}

int apply(const char *value, const char *path) {
    return write_value(value, path, 1);
}

int write_file(const char *value, const char *path) {
    return write_value(value, path, 0);
}

int apply_ll(long long value, const char *path) {
    char str[64];
    snprintf(str, sizeof(str), "%lld", value);
    return apply(str, path);
}

int write_ll(long long value, const char *path) {
    char str[64];
    snprintf(str, sizeof(str), "%lld", value);
    return write_file(str, path);
}

bool name_contains(const char *name, const char *needle) {
    return name && needle && strstr(name, needle) != NULL;
}

bool name_starts_with(const char *name, const char *prefix) {
    return name && prefix && strncmp(name, prefix, strlen(prefix)) == 0;
}

bool name_is_policy(const char *name, void *ctx) {
    (void)ctx;
    return name_starts_with(name, "policy") && isdigit((unsigned char)name[6]);
}

bool name_has_token(const char *name, void *ctx) {
    const char *const *tokens = ctx;
    if (!name || !tokens) return false;
    for (size_t i = 0; tokens[i]; ++i) {
        if (strstr(name, tokens[i])) return true;
    }
    return false;
}

int for_each_dir_entry(const char *dir, velfox_entry_matcher matcher, velfox_entry_handler handler, void *ctx) {
    if (!dir || !handler) return 0;
    DIR *dp = opendir(dir);
    if (!dp) return 0;
    int count = 0;
    struct dirent *ent;
    while ((ent = readdir(dp)) != NULL) {
        const char *name = ent->d_name;
        if (!name[0] || name[0] == '.') continue;
        if (matcher && !matcher(name, ctx)) continue;
        handler(dir, name, ctx);
        ++count;
    }
    closedir(dp);
    return count;
}

int path_join(char *out, size_t size, const char *dir, const char *name) {
    if (!out || !dir || !name || size == 0) return 0;
    return snprintf(out, size, "%s/%s", dir, name) < (int)size;
}

int path_join3(char *out, size_t size, const char *dir, const char *name, const char *leaf) {
    if (!out || !dir || !name || !leaf || size == 0) return 0;
    return snprintf(out, size, "%s/%s/%s", dir, name, leaf) < (int)size;
}
