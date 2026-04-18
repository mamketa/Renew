#pragma once
#include "Common.hpp"

namespace velfox {

class DevFreq {
public:
    static bool maxPerf(std::string_view path) noexcept;
    static bool midPerf(std::string_view path) noexcept;
    static bool unlock(std::string_view path) noexcept;
    static bool minPerf(std::string_view path) noexcept;
    static bool qcomMaxPerf(std::string_view path) noexcept;
    static bool qcomMidPerf(std::string_view path) noexcept;
    static bool qcomUnlock(std::string_view path) noexcept;
    static bool qcomMinPerf(std::string_view path) noexcept;
    static int  mtkGpuMinFreqIndex(std::string_view path) noexcept;
    static int  mtkGpuMidFreqIndex(std::string_view path) noexcept;
    static std::vector<long> mtkParseGpuFreqTable(std::string_view path) noexcept;
};

} // namespace velfox
