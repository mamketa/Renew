#ifndef VELFOX_COMMON_H
#define VELFOX_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

#define MODULE_CONFIG "/data/adb/.config/Velfox"
#define MAX_PATH_LEN 256
#define MAX_LINE_LEN 1024
#define MAX_OPP_COUNT 50

extern int SOC;
extern int LITE_MODE;
extern int DEVICE_MITIGATION;
extern char DEFAULT_CPU_GOV[50];
extern char PPM_POLICY[512];

typedef int (*sysfs_iter_cb)(const char *entry_path, const char *entry_name, void *userdata);

int file_exists(const char *path);
int path_join(char *out, size_t size, const char *base, const char *name);
int read_int_from_file(const char *path);
void read_string_from_file(char *buffer, size_t size, const char *path);
int string_has_token(const char *haystack, const char *needle);
int sysfs_value_supported(const char *path, const char *value, char *matched, size_t matched_size);
int write_sysfs(const char *path, const char *value);
int apply(const char *value, const char *path);
int write_file(const char *value, const char *path);
int apply_ll(long long value, const char *path);
int write_ll(long long value, const char *path);
int iterate_sysfs_nodes(const char *base_dir, const char *name_contains, sysfs_iter_cb callback, void *userdata);
int apply_sysfs_child_value(const char *base_dir, const char *name_contains, const char *leaf, const char *value);

#endif
