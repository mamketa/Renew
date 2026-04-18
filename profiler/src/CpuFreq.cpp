#include "CpuFreq.hpp"
#include "FileIO.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <algorithm>
#include <format>
#include <print>

namespace velfox {

int CpuFreq::getCpuCount() noexcept {
    DIR* dir = ::opendir("/sys/devices/system/cpu");
    if (!dir) return 0;
    int count = 0;
    struct dirent* e;
    while ((e = ::readdir(dir)))
        if (::strncmp(e->d_name, "cpu", 3) == 0 && ::isdigit(e->d_name[3]))
            ++count;
    ::closedir(dir);
    return count;
}

void CpuFreq::changeGov(std::string_view gov) noexcept {
    int n = getCpuCount();
    for (int i = 0; i < n; ++i) {
        auto p = std::format("/sys/devices/system/cpu/cpu{}/cpufreq/scaling_governor", i);
        if (FileIO::exists(p)) {
            ::chmod(p.c_str(), 0644);
            FileIO::write(gov, p);
        }
    }
    DIR* dir = ::opendir("/sys/devices/system/cpu/cpufreq");
    if (!dir) return;
    struct dirent* e;
    while ((e = ::readdir(dir))) {
        if (::strstr(e->d_name, "policy")) {
            auto p = std::format("/sys/devices/system/cpu/cpufreq/{}/scaling_governor", e->d_name);
            if (FileIO::exists(p)) ::chmod(p.c_str(), 0644);
        }
    }
    ::closedir(dir);
}

long CpuFreq::getMaxFreq(std::string_view path) noexcept {
    FILE* fp = ::fopen(path.data(), "r");
    if (!fp) return 0;
    long mx = 0, f;
    while (::fscanf(fp, "%ld", &f) == 1) if (f > mx) mx = f;
    ::fclose(fp);
    return mx;
}

long CpuFreq::getMinFreq(std::string_view path) noexcept {
    FILE* fp = ::fopen(path.data(), "r");
    if (!fp) return 0;
    long mn = LONG_MAX, f;
    while (::fscanf(fp, "%ld", &f) == 1) if (f > 0 && f < mn) mn = f;
    ::fclose(fp);
    return (mn == LONG_MAX) ? 0 : mn;
}

long CpuFreq::getMidFreq(std::string_view path) noexcept {
    FILE* fp = ::fopen(path.data(), "r");
    if (!fp) return 0;
    std::vector<long> freqs;
    freqs.reserve(MAX_OPP_COUNT);
    long f;
    while ((int)freqs.size() < MAX_OPP_COUNT && ::fscanf(fp, "%ld", &f) == 1)
        freqs.push_back(f);
    ::fclose(fp);
    if (freqs.empty()) return 0;
    std::sort(freqs.begin(), freqs.end());
    return freqs[freqs.size() / 2];
}

void CpuFreq::ppmMaxPerf(int liteMode) noexcept {
    DIR* dir = ::opendir("/sys/devices/system/cpu/cpufreq");
    if (!dir) return;
    struct dirent* e;
    int cluster = 0;
    while ((e = ::readdir(dir))) {
        if (!::strstr(e->d_name, "policy")) continue;
        auto maxPath = std::format("/sys/devices/system/cpu/cpufreq/{}/cpuinfo_max_freq", e->d_name);
        long cpu_max = FileIO::readInt(maxPath);
        if (FileIO::exists("/proc/ppm/policy/hard_userlimit_max_cpu_freq")) {
            auto cmd = std::format("{} {}", cluster, cpu_max);
            FileIO::apply(cmd, "/proc/ppm/policy/hard_userlimit_max_cpu_freq");
        }
        if (FileIO::exists("/proc/ppm/policy/hard_userlimit_min_cpu_freq")) {
            long target = cpu_max;
            if (liteMode) {
                auto avail = std::format("/sys/devices/system/cpu/cpufreq/{}/scaling_available_frequencies", e->d_name);
                long mid = getMidFreq(avail);
                if (mid > 0) target = mid;
            }
            auto cmd = std::format("{} {}", cluster, target);
            FileIO::apply(cmd, "/proc/ppm/policy/hard_userlimit_min_cpu_freq");
        }
        ++cluster;
    }
    ::closedir(dir);
}

void CpuFreq::maxPerf(int liteMode) noexcept {
    int n = getCpuCount();
    for (int i = 0; i < n; ++i) {
        auto maxInfoPath = std::format("/sys/devices/system/cpu/cpu{}/cpufreq/cpuinfo_max_freq", i);
        if (!FileIO::exists(maxInfoPath)) continue;
        long cpu_max = FileIO::readInt(maxInfoPath);
        FileIO::applyLL(cpu_max, std::format("/sys/devices/system/cpu/cpu{}/cpufreq/scaling_max_freq", i));
        long target = cpu_max;
        if (liteMode) {
            auto avail = std::format("/sys/devices/system/cpu/cpu{}/cpufreq/scaling_available_frequencies", i);
            long mid = getMidFreq(avail);
            if (mid > 0) target = mid;
        }
        FileIO::applyLL(target, std::format("/sys/devices/system/cpu/cpu{}/cpufreq/scaling_min_freq", i));
    }
    DIR* dir = ::opendir("/sys/devices/system/cpu/cpufreq");
    if (!dir) return;
    struct dirent* e;
    while ((e = ::readdir(dir))) {
        if (!::strstr(e->d_name, "policy")) continue;
        ::chmod(std::format("/sys/devices/system/cpu/cpufreq/{}/scaling_max_freq", e->d_name).c_str(), 0644);
        ::chmod(std::format("/sys/devices/system/cpu/cpufreq/{}/scaling_min_freq", e->d_name).c_str(), 0644);
    }
    ::closedir(dir);
}

void CpuFreq::ppmUnlock() noexcept {
    DIR* dir = ::opendir("/sys/devices/system/cpu/cpufreq");
    if (!dir) return;
    struct dirent* e;
    int cluster = 0;
    while ((e = ::readdir(dir))) {
        if (!::strstr(e->d_name, "policy")) continue;
        long cpu_max = FileIO::readInt(std::format("/sys/devices/system/cpu/cpufreq/{}/cpuinfo_max_freq", e->d_name));
        long cpu_min = FileIO::readInt(std::format("/sys/devices/system/cpu/cpufreq/{}/cpuinfo_min_freq", e->d_name));
        if (FileIO::exists("/proc/ppm/policy/hard_userlimit_max_cpu_freq"))
            FileIO::write(std::format("{} {}", cluster, cpu_max), "/proc/ppm/policy/hard_userlimit_max_cpu_freq");
        if (FileIO::exists("/proc/ppm/policy/hard_userlimit_min_cpu_freq"))
            FileIO::write(std::format("{} {}", cluster, cpu_min), "/proc/ppm/policy/hard_userlimit_min_cpu_freq");
        ++cluster;
    }
    ::closedir(dir);
}

void CpuFreq::unlock() noexcept {
    int n = getCpuCount();
    for (int i = 0; i < n; ++i) {
        auto infoMax = std::format("/sys/devices/system/cpu/cpu{}/cpufreq/cpuinfo_max_freq", i);
        if (!FileIO::exists(infoMax)) continue;
        long cpu_max = FileIO::readInt(infoMax);
        long cpu_min = FileIO::readInt(std::format("/sys/devices/system/cpu/cpu{}/cpufreq/cpuinfo_min_freq", i));
        FileIO::writeLL(cpu_max, std::format("/sys/devices/system/cpu/cpu{}/cpufreq/scaling_max_freq", i));
        FileIO::writeLL(cpu_min, std::format("/sys/devices/system/cpu/cpu{}/cpufreq/scaling_min_freq", i));
    }
    DIR* dir = ::opendir("/sys/devices/system/cpu/cpufreq");
    if (!dir) return;
    struct dirent* e;
    while ((e = ::readdir(dir))) {
        if (!::strstr(e->d_name, "policy")) continue;
        ::chmod(std::format("/sys/devices/system/cpu/cpufreq/{}/scaling_max_freq", e->d_name).c_str(), 0644);
        ::chmod(std::format("/sys/devices/system/cpu/cpufreq/{}/scaling_min_freq", e->d_name).c_str(), 0644);
    }
    ::closedir(dir);
}

} // namespace velfox
