/*
 * utility_utils.c — Core sysfs/procfs utility functions for Velfox Profiler
 *
 * Provides the foundational I/O primitives used by every other module.
 * The central function, write_sysfs(), enforces a strict defensive contract:
 *   1. Null / empty-string guard on all pointer arguments.
 *   2. Node existence check via access(F_OK) before any open() attempt.
 *   3. Idempotent write: reads current value and skips write if unchanged.
 *   4. Write-permission check via access(W_OK) before open().
 *   5. O_CLOEXEC on every open() call — no file-descriptor leaks into children.
 *   6. File descriptor always closed before function returns, success or not.
 *   7. fsync() called only when the full byte count was confirmed written.
 *
 * Standard: C23 / Clang
 */

#include "velfox_common.h"

/* -------------------------------------------------------------------------
 * file_exists
 * ---------------------------------------------------------------------- */
int file_exists(const char *path) {
    if (!path || !path[0]) return 0;
    return access(path, F_OK) == 0;
}

/* -------------------------------------------------------------------------
 * path_join
 * ---------------------------------------------------------------------- */
int path_join(char *out, size_t size, const char *base, const char *name) {
    if (!out || size == 0 || !base || !name) return 0;
    int written = snprintf(out, size, "%s/%s", base, name);
    return written > 0 && (size_t)written < size;
}

/* -------------------------------------------------------------------------
 * read_int_from_file
 * ---------------------------------------------------------------------- */
int read_int_from_file(const char *path) {
    if (!path || !path[0]) return 0;
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    int value = 0;
    if (fscanf(fp, "%d", &value) != 1) value = 0;
    fclose(fp);
    return value;
}

/* -------------------------------------------------------------------------
 * read_string_from_file
 * ---------------------------------------------------------------------- */
void read_string_from_file(char *buffer, size_t size, const char *path) {
    if (!buffer || size == 0) return;
    buffer[0] = '\0';
    if (!path || !path[0]) return;
    FILE *fp = fopen(path, "r");
    if (!fp) return;
    if (fgets(buffer, (int)size, fp)) {
        buffer[strcspn(buffer, "\r\n")] = '\0';
    }
    fclose(fp);
}

/* -------------------------------------------------------------------------
 * string_has_token — internal helper
 * ---------------------------------------------------------------------- */
static int token_boundary(char c) {
    return c == '\0'
        || isspace((unsigned char)c)
        || c == '[' || c == ']'
        || c == ',' || c == ';' || c == ':';
}

int string_has_token(const char *haystack, const char *needle) {
    if (!haystack || !needle || !needle[0]) return 0;
    size_t needle_len = strlen(needle);
    const char *pos = haystack;
    while ((pos = strstr(pos, needle)) != NULL) {
        char before = (pos == haystack) ? '\0' : pos[-1];
        char after  = pos[needle_len];
        if (token_boundary(before) && token_boundary(after)) return 1;
        pos += needle_len;
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * sysfs_value_supported
 * ---------------------------------------------------------------------- */
int sysfs_value_supported(const char *path,
                           const char *value,
                           char       *matched,
                           size_t      matched_size) {
    if (matched && matched_size > 0) matched[0] = '\0';
    if (!path || !path[0] || !value) return 0;

    FILE *fp = fopen(path, "r");
    if (!fp) return 0;

    char line[MAX_LINE_LEN];
    int found = 0;

    while (!found && fgets(line, sizeof(line), fp)) {
        char *save  = NULL;
        char *token = strtok_r(line, " \t\r\n", &save);
        while (token) {
            size_t len = strlen(token);
            /* Strip surrounding brackets [value] */
            if (len >= 2 && token[0] == '[' && token[len - 1] == ']') {
                token[len - 1] = '\0';
                ++token;
                len -= 2;
            }
            if (strcmp(token, value) == 0 || strstr(token, value) != NULL) {
                if (matched && matched_size > 0) {
                    snprintf(matched, matched_size, "%s", token);
                }
                found = 1;
                break;
            }
            token = strtok_r(NULL, " \t\r\n", &save);
        }
    }

    fclose(fp);
    return found;
}

/* -------------------------------------------------------------------------
 * write_sysfs — defensive sysfs/procfs write wrapper
 *
 * This is the single authoritative function for all kernel node writes.
 * Every other module calls this function; no module opens sysfs paths
 * directly.  See the contract at the top of this file.
 * ---------------------------------------------------------------------- */
int write_sysfs(const char *path, const char *value) {
    /* 1. Argument validation */
    if (!path || !path[0] || !value) return 0;

    /* 2. Node existence check — do not attempt open on missing nodes */
    if (access(path, F_OK) != 0) return 0;

    /* 3. Idempotent write: skip if current value already matches */
    char current[MAX_LINE_LEN];
    read_string_from_file(current, sizeof(current), path);
    if (current[0] && strcmp(current, value) == 0) return 1;

    /* 4. Write-permission check */
    if (access(path, W_OK) != 0) return 0;

    /* 5. Open with O_CLOEXEC to prevent FD leak into child processes */
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd < 0) return 0;

    /* 6. Write and conditionally sync */
    size_t  len     = strlen(value);
    ssize_t written = write(fd, value, len);
    int     success = (written == (ssize_t)len);
    if (success) fsync(fd);

    /* 7. Always close — no early returns beyond this point */
    close(fd);
    return success;
}

/* -------------------------------------------------------------------------
 * Compatibility aliases
 * ---------------------------------------------------------------------- */
int apply(const char *value, const char *path) {
    return write_sysfs(path, value);
}

int write_file(const char *value, const char *path) {
    return write_sysfs(path, value);
}

int apply_ll(long long value, const char *path) {
    char str[64];
    snprintf(str, sizeof(str), "%lld", value);
    return write_sysfs(path, str);
}

int write_ll(long long value, const char *path) {
    char str[64];
    snprintf(str, sizeof(str), "%lld", value);
    return write_sysfs(path, str);
}

/* -------------------------------------------------------------------------
 * iterate_sysfs_nodes
 * ---------------------------------------------------------------------- */
int iterate_sysfs_nodes(const char    *base_dir,
                        const char    *name_contains,
                        sysfs_iter_cb  callback,
                        void          *userdata) {
    if (!base_dir || !callback) return 0;

    DIR *dir = opendir(base_dir);
    if (!dir) return 0;

    int processed = 0;
    struct dirent *ent;

    while ((ent = readdir(dir)) != NULL) {
        /* Skip hidden entries and self/parent references */
        if (!ent->d_name[0] || ent->d_name[0] == '.') continue;
        if (name_contains && !strstr(ent->d_name, name_contains)) continue;

        char entry_path[MAX_PATH_LEN];
        if (!path_join(entry_path, sizeof(entry_path), base_dir, ent->d_name)) continue;

        processed += callback(entry_path, ent->d_name, userdata) ? 1 : 0;
    }

    closedir(dir);
    return processed;
}

/* -------------------------------------------------------------------------
 * apply_sysfs_child_value
 * ---------------------------------------------------------------------- */
struct apply_child_data {
    const char *leaf;
    const char *value;
};

static int apply_child_cb(const char *entry_path,
                           const char *entry_name,
                           void       *userdata) {
    (void)entry_name;
    struct apply_child_data *data = (struct apply_child_data *)userdata;
    char path[MAX_PATH_LEN];
    if (!path_join(path, sizeof(path), entry_path, data->leaf)) return 0;
    return write_sysfs(path, data->value);
}

int apply_sysfs_child_value(const char *base_dir,
                             const char *name_contains,
                             const char *leaf,
                             const char *value) {
    struct apply_child_data data = { leaf, value };
    return iterate_sysfs_nodes(base_dir, name_contains, apply_child_cb, &data);
}
