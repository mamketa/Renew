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

#pragma once

#include "velfox_common.h"

/* Node existence check */
int  node_exists(const char *path);

/* Probe multiple candidate paths, return first found or NULL */
const char *probe_paths(const char *const *paths, int count);

/* Read helpers */
int  read_int_from_file(const char *path);
void read_string_from_file(char *buffer, size_t size, const char *path);

/*
 * safe_write: attempt write without chmod first.
 * Falls back to chmod(0644) only if initial open fails.
 * Silently skips if node is missing.
 * Skips if value already matches current content (no unnecessary writes).
 * Restores chmod(0444) after write (sysfs convention).
 */
int safe_write(const char *path, const char *value);

/*
 * apply: same as safe_write, aliased for backward compatibility.
 * Retained so all existing callers compile without change.
 */
int apply(const char *value, const char *path);

/*
 * write_file: identical behavior to safe_write/apply.
 * Retained for callers that use this name.
 */
int write_file(const char *value, const char *path);

/* Numeric helpers */
int apply_ll(long long value, const char *path);
int write_ll(long long value, const char *path);
