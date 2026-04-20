/*
 * velfox_common.h — Core types, constants, and utility function declarations
 *
 * This header is the base inclusion for every translation unit in the project.
 * It pulls in the standard library headers required project-wide and declares
 * the shared utility API implemented in utility_utils.c.
 *
 * Standard: C23 / Clang
 */

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

#include "velfox_paths.h"

/* -------------------------------------------------------------------------
 * Project-wide buffer size constants
 * ---------------------------------------------------------------------- */
#define MAX_PATH_LEN    256
#define MAX_LINE_LEN    1024
#define MAX_OPP_COUNT   50

/* -------------------------------------------------------------------------
 * Global runtime state — defined in modes_utils.c, declared extern here
 * ---------------------------------------------------------------------- */
extern int SOC;
extern int LITE_MODE;
extern int DEVICE_MITIGATION;
extern char DEFAULT_CPU_GOV[50];
extern char PPM_POLICY[512];

/* -------------------------------------------------------------------------
 * Callback type for iterate_sysfs_nodes()
 *
 * @param entry_path  Full path to the directory entry (base_dir + "/" + name)
 * @param entry_name  Bare directory entry name (d_name)
 * @param userdata    Caller-supplied context pointer (may be NULL)
 * @return            Non-zero on success, 0 if the entry was skipped/failed
 * ---------------------------------------------------------------------- */
typedef int (*sysfs_iter_cb)(const char *entry_path,
                             const char *entry_name,
                             void       *userdata);

/* -------------------------------------------------------------------------
 * Utility function declarations (implemented in utility_utils.c)
 * ---------------------------------------------------------------------- */

/*
 * file_exists() — Test whether a filesystem path is accessible.
 * Uses access(path, F_OK) to avoid TOCTOU confusion with stat().
 * Returns 1 if the path exists and is accessible, 0 otherwise.
 */
int file_exists(const char *path);

/*
 * path_join() — Safely concatenate base + "/" + name into out.
 * Returns 1 on success (no truncation), 0 on error or truncation.
 */
int path_join(char *out, size_t size, const char *base, const char *name);

/*
 * read_int_from_file() — Read a single integer from a sysfs/procfs node.
 * Returns the integer value, or 0 on any error.
 */
int read_int_from_file(const char *path);

/*
 * read_string_from_file() — Read one line from a sysfs/procfs node into buffer.
 * Strips trailing CR/LF.  buffer is always NUL-terminated on return.
 */
void read_string_from_file(char *buffer, size_t size, const char *path);

/*
 * string_has_token() — Check whether needle appears as a whole token in haystack.
 * Tokens are delimited by whitespace, brackets, commas, semicolons, or colons.
 * Returns 1 if found, 0 otherwise.
 */
int string_has_token(const char *haystack, const char *needle);

/*
 * sysfs_value_supported() — Check whether a sysfs node lists value as supported.
 * Reads the node contents and looks for value as a whitespace/bracket-delimited
 * token.  If matched is non-NULL and matched_size > 0 the raw matched token
 * (with brackets stripped) is written there.
 * Returns 1 if supported, 0 otherwise.
 */
int sysfs_value_supported(const char *path,
                           const char *value,
                           char       *matched,
                           size_t      matched_size);

/*
 * write_sysfs() — Defensive sysfs/procfs write wrapper.
 *
 * Execution contract:
 *  1. Validates path and value are non-NULL / non-empty.
 *  2. Confirms node existence via access(path, F_OK) — no write attempted
 *     if the node is absent (returns 0 silently, never crashes).
 *  3. Reads current value and short-circuits if it already matches (idempotent).
 *  4. Confirms write permission via access(path, W_OK).
 *  5. Opens the node with O_WRONLY | O_CLOEXEC; never leaks the descriptor.
 *  6. Calls fsync() only on confirmed full writes.
 *  7. Always closes the file descriptor before returning.
 *
 * Returns 1 on success, 0 on any failure.
 */
int write_sysfs(const char *path, const char *value);

/*
 * apply() / write_file() — Aliases for write_sysfs() with swapped argument
 * order, preserved for compatibility with existing SOC-specific modules.
 */
int apply(const char *value, const char *path);
int write_file(const char *value, const char *path);

/*
 * apply_ll() / write_ll() — write_sysfs() wrappers that format a long long
 * integer into a decimal string before writing.
 */
int apply_ll(long long value, const char *path);
int write_ll(long long value, const char *path);

/*
 * iterate_sysfs_nodes() — Walk entries in base_dir, optionally filtering by
 * name_contains substring, and invoke callback for each matching entry.
 * Returns the total count of entries for which callback returned non-zero.
 */
int iterate_sysfs_nodes(const char    *base_dir,
                        const char    *name_contains,
                        sysfs_iter_cb  callback,
                        void          *userdata);

/*
 * apply_sysfs_child_value() — For each entry in base_dir matching
 * name_contains, write value to the relative leaf sub-path.
 * Convenience wrapper around iterate_sysfs_nodes().
 */
int apply_sysfs_child_value(const char *base_dir,
                             const char *name_contains,
                             const char *leaf,
                             const char *value);

#endif /* VELFOX_COMMON_H */
