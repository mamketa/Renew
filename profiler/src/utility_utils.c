#include "velfox_common.h"

int file_exists(const char *path) {
    if (!path || !path[0]) return 0;
    return access(path, F_OK) == 0;
}

int path_join(char *out, size_t size, const char *base, const char *name) {
    if (!out || size == 0 || !base || !name) return 0;
    int written = snprintf(out, size, "%s/%s", base, name);
    return written > 0 && written < (int)size;
}

int read_int_from_file(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    int value = 0;
    if (fscanf(fp, "%d", &value) != 1) value = 0;
    fclose(fp);
    return value;
}

void read_string_from_file(char *buffer, size_t size, const char *path) {
    if (!buffer || size == 0) return;
    buffer[0] = '\0';
    if (!path) return;
    FILE *fp = fopen(path, "r");
    if (!fp) return;
    if (fgets(buffer, size, fp)) buffer[strcspn(buffer, "\r\n")] = '\0';
    fclose(fp);
}

static int token_boundary(char c) {
    return c == '\0' || isspace((unsigned char)c) || c == '[' || c == ']' || c == ',' || c == ';' || c == ':';
}

int string_has_token(const char *haystack, const char *needle) {
    if (!haystack || !needle || !needle[0]) return 0;
    size_t needle_len = strlen(needle);
    const char *pos = haystack;
    while ((pos = strstr(pos, needle)) != NULL) {
        char before = (pos == haystack) ? '\0' : pos[-1];
        char after = pos[needle_len];
        if (token_boundary(before) && token_boundary(after)) return 1;
        pos += needle_len;
    }
    return 0;
}

int sysfs_value_supported(const char *path, const char *value, char *matched, size_t matched_size) {
    if (matched && matched_size > 0) matched[0] = '\0';
    if (!path || !value) return 0;
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    char line[MAX_LINE_LEN];
    int found = 0;
    while (fgets(line, sizeof(line), fp)) {
        char *save = NULL;
        char *token = strtok_r(line, " \t\r\n", &save);
        while (token) {
            size_t len = strlen(token);
            if (len >= 2 && token[0] == '[' && token[len - 1] == ']') {
                token[len - 1] = '\0';
                token++;
                len -= 2;
            }
            if (strcmp(token, value) == 0 || strstr(token, value) != NULL) {
                if (matched && matched_size > 0) snprintf(matched, matched_size, "%s", token);
                found = 1;
                break;
            }
            token = strtok_r(NULL, " \t\r\n", &save);
        }
        if (found) break;
    }
    fclose(fp);
    return found;
}

int write_sysfs(const char *path, const char *value) {
    if (!path || !value || !path[0]) return 0;
    if (access(path, F_OK) != 0) return 0;
    char current[MAX_LINE_LEN];
    read_string_from_file(current, sizeof(current), path);
    if (current[0] && strcmp(current, value) == 0) return 1;
    if (access(path, W_OK) != 0 && errno == EACCES) return 0;
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd < 0) return 0;
    size_t len = strlen(value);
    ssize_t written = write(fd, value, len);
    if (written == (ssize_t)len) fsync(fd);
    close(fd);
    return written == (ssize_t)len;
}

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

int iterate_sysfs_nodes(const char *base_dir, const char *name_contains, sysfs_iter_cb callback, void *userdata) {
    if (!base_dir || !callback) return 0;
    DIR *dir = opendir(base_dir);
    if (!dir) return 0;
    int processed = 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (!ent->d_name[0] || ent->d_name[0] == '.') continue;
        if (name_contains && !strstr(ent->d_name, name_contains)) continue;
        char path[MAX_PATH_LEN];
        if (!path_join(path, sizeof(path), base_dir, ent->d_name)) continue;
        processed += callback(path, ent->d_name, userdata) ? 1 : 0;
    }
    closedir(dir);
    return processed;
}

struct apply_child_data {
    const char *leaf;
    const char *value;
};

static int apply_child_cb(const char *entry_path, const char *entry_name, void *userdata) {
    (void)entry_name;
    struct apply_child_data *data = (struct apply_child_data *)userdata;
    char path[MAX_PATH_LEN];
    if (!path_join(path, sizeof(path), entry_path, data->leaf)) return 0;
    return write_sysfs(path, data->value);
}

int apply_sysfs_child_value(const char *base_dir, const char *name_contains, const char *leaf, const char *value) {
    struct apply_child_data data = {leaf, value};
    return iterate_sysfs_nodes(base_dir, name_contains, apply_child_cb, &data);
}
