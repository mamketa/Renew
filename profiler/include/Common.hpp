#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
#include <climits>
#include <cstdint>
#include <print>

namespace fs = std::filesystem;

inline constexpr std::string_view MODULE_CONFIG = "/data/adb/.config/Velfox";
inline constexpr int MAX_OPP_COUNT = 50;
