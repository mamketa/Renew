#include "DevFreq.hpp"
#include "FileIO.hpp"
#include "CpuFreq.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <format>

namespace velfox {

bool DevFreq::maxPerf(std::string_view path) noexcept {
    if (!FileIO::exists(path)) return false;
    auto fp = std::format("{}/available_frequencies", path);
    if (!FileIO::exists(fp)) return false;
    long mx = CpuFreq::getMaxFreq(fp);
    if (mx <= 0) return false;
    FileIO::applyLL(mx, std::format("{}/max_freq", path));
    FileIO::applyLL(mx, std::format("{}/min_freq", path));
    return true;
}

bool DevFreq::midPerf(std::string_view path) noexcept {
    if (!FileIO::exists(path)) return false;
    auto fp = std::format("{}/available_frequencies", path);
    if (!FileIO::exists(fp)) return false;
    long mx = CpuFreq::getMaxFreq(fp);
    long mid = CpuFreq::getMidFreq(fp);
    if (mx <= 0 || mid <= 0) return false;
    FileIO::applyLL(mx, std::format("{}/max_freq", path));
    FileIO::applyLL(mid, std::format("{}/min_freq", path));
    return true;
}

bool DevFreq::unlock(std::string_view path) noexcept {
    if (!FileIO::exists(path)) return false;
    auto fp = std::format("{}/available_frequencies", path);
    if (!FileIO::exists(fp)) return false;
    long mx = CpuFreq::getMaxFreq(fp);
    long mn = CpuFreq::getMinFreq(fp);
    if (mx <= 0 || mn <= 0) return false;
    FileIO::writeLL(mx, std::format("{}/max_freq", path));
    FileIO::writeLL(mn, std::format("{}/min_freq", path));
    return true;
}

bool DevFreq::minPerf(std::string_view path) noexcept {
    if (!FileIO::exists(path)) return false;
    auto fp = std::format("{}/available_frequencies", path);
    if (!FileIO::exists(fp)) return false;
    long mn = CpuFreq::getMinFreq(fp);
    if (mn <= 0) return false;
    FileIO::applyLL(mn, std::format("{}/min_freq", path));
    FileIO::applyLL(mn, std::format("{}/max_freq", path));
    return true;
}

bool DevFreq::qcomMaxPerf(std::string_view path) noexcept {
    if (!FileIO::exists(path)) return false;
    auto fp = std::format("{}/available_frequencies", path);
    if (!FileIO::exists(fp)) return false;
    long f = CpuFreq::getMaxFreq(fp);
    FileIO::applyLL(f, std::format("{}/hw_max_freq", path));
    FileIO::applyLL(f, std::format("{}/hw_min_freq", path));
    return true;
}

bool DevFreq::qcomMidPerf(std::string_view path) noexcept {
    if (!FileIO::exists(path)) return false;
    auto fp = std::format("{}/available_frequencies", path);
    if (!FileIO::exists(fp)) return false;
    long mx  = CpuFreq::getMaxFreq(fp);
    long mid = CpuFreq::getMidFreq(fp);
    FileIO::applyLL(mx,  std::format("{}/hw_max_freq", path));
    FileIO::applyLL(mid, std::format("{}/hw_min_freq", path));
    return true;
}

bool DevFreq::qcomUnlock(std::string_view path) noexcept {
    if (!FileIO::exists(path)) return false;
    auto fp = std::format("{}/available_frequencies", path);
    if (!FileIO::exists(fp)) return false;
    long mx = CpuFreq::getMaxFreq(fp);
    long mn = CpuFreq::getMinFreq(fp);
    FileIO::writeLL(mx, std::format("{}/hw_max_freq", path));
    FileIO::writeLL(mn, std::format("{}/hw_min_freq", path));
    return true;
}

bool DevFreq::qcomMinPerf(std::string_view path) noexcept {
    if (!FileIO::exists(path)) return false;
    auto fp = std::format("{}/available_frequencies", path);
    if (!FileIO::exists(fp)) return false;
    long f = CpuFreq::getMinFreq(fp);
    FileIO::applyLL(f, std::format("{}/hw_min_freq", path));
    FileIO::applyLL(f, std::format("{}/hw_max_freq", path));
    return true;
}

std::vector<long> DevFreq::mtkParseGpuFreqTable(std::string_view path) noexcept {
    std::vector<long> freqs;
    FILE* fp = ::fopen(path.data(), "r");
    if (!fp) return freqs;
    char line[1024];
    while (::fgets(line, sizeof(line), fp) && (int)freqs.size() < MAX_OPP_COUNT) {
        char* s = ::strchr(line, '[');
        char* e = ::strchr(line, ']');
        if (!s || !e || e <= s) continue;
        char tmp[16];
        int len = (int)(e - s - 1);
        if (len <= 0 || len >= (int)sizeof(tmp)) continue;
        ::memcpy(tmp, s + 1, len);
        tmp[len] = '\0';
        freqs.push_back(::atol(tmp));
    }
    ::fclose(fp);
    return freqs;
}

int DevFreq::mtkGpuMinFreqIndex(std::string_view path) noexcept {
    auto v = mtkParseGpuFreqTable(path);
    return v.empty() ? 0 : (int)v.back();
}

int DevFreq::mtkGpuMidFreqIndex(std::string_view path) noexcept {
    auto v = mtkParseGpuFreqTable(path);
    return v.empty() ? 0 : (int)v[v.size() / 2];
}

} // namespace velfox
