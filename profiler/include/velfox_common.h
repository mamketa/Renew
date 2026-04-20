#ifndef VELFOX_COMMON_H
#define VELFOX_COMMON_H

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MODULE_CONFIG "/data/adb/.config/Velfox"
#define MAX_PATH_LEN 256
#define MAX_LINE_LEN 1024
#define MAX_OPP_COUNT 50

extern int SOC;
extern int LITE_MODE;
extern int DEVICE_MITIGATION;
extern char DEFAULT_CPU_GOV[50];
extern char PPM_POLICY[512];

typedef bool (*velfox_entry_matcher)(const char *name, void *ctx);
typedef void (*velfox_entry_handler)(const char *dir, const char *name, void *ctx);

int file_exists(const char *path);
int read_int_from_file(const char *path);
void read_string_from_file(char *buffer, size_t size, const char *path);
int apply(const char *value, const char *path);
int write_file(const char *value, const char *path);
int apply_ll(long long value, const char *path);
int write_ll(long long value, const char *path);
bool name_contains(const char *name, const char *needle);
bool name_starts_with(const char *name, const char *prefix);
bool name_is_policy(const char *name, void *ctx);
bool name_has_token(const char *name, void *ctx);
int for_each_dir_entry(const char *dir, velfox_entry_matcher matcher, velfox_entry_handler handler, void *ctx);
int path_join(char *out, size_t size, const char *dir, const char *name);
int path_join3(char *out, size_t size, const char *dir, const char *name, const char *leaf);

#endif
