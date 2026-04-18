#pragma once
#include "Common.hpp"

namespace velfox {

class CpuFreq {
public:
    static int  getCpuCount() noexcept;
    static void changeGov(std::string_view gov) noexcept;
    static long getMaxFreq(std::string_view path) noexcept;
    static long getMinFreq(std::string_view path) noexcept;
    static long getMidFreq(std::string_view path) noexcept;
    static void ppmMaxPerf(int liteMode) noexcept;
    static void maxPerf(int liteMode) noexcept;
    static void ppmUnlock() noexcept;
    static void unlock() noexcept;
};

} // namespace velfox
