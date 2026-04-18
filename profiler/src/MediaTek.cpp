#include "SocManager.hpp"
#include "FileIO.hpp"
#include "CpuFreq.hpp"
#include "DevFreq.hpp"
#include <dirent.h>
#include <cstdio>
#include <cstring>
#include <climits>
#include <format>
#include <sys/types.h>

namespace velfox {

class MediaTekManager final : public SocManager {
public:
    explicit MediaTekManager(const Config& cfg) : cfg_(cfg) {}

    void applyEsport() override {
        applyPpmPolicies(0);

        FileIO::apply("0", "/sys/kernel/fpsgo/fstb/adopt_low_fps");
        FileIO::apply("1", "/sys/pnpmgr/fpsgo_boost/boost_enable");
        FileIO::apply("1", "/sys/kernel/fpsgo/fstb/gpu_slowdown_check");
        FileIO::apply("1", "/proc/cpufreq/cpufreq_cci_mode");
        FileIO::apply("3", "/proc/cpufreq/cpufreq_power_mode");
        FileIO::apply("1", "/proc/perfmgr/syslimiter/syslimiter_force_disable");
        FileIO::apply("1", "/sys/devices/platform/boot_dramboost/dramboost/dramboost");
        FileIO::apply("0", "/sys/devices/system/cpu/eas/enable");

        applyGpuEsport();

        FileIO::apply("ignore_batt_oc 1",         "/proc/gpufreq/gpufreq_power_limited");
        FileIO::apply("ignore_batt_percent 1",     "/proc/gpufreq/gpufreq_power_limited");
        FileIO::apply("ignore_low_batt 1",         "/proc/gpufreq/gpufreq_power_limited");
        FileIO::apply("ignore_thermal_protect 1",  "/proc/gpufreq/gpufreq_power_limited");
        FileIO::apply("ignore_pbm_limited 1",      "/proc/gpufreq/gpufreq_power_limited");
        FileIO::apply("stop 1", "/proc/mtk_batoc_throttling/battery_oc_protect_stop");

        applyDramEsport();
        FileIO::apply("0", "/sys/kernel/eara_thermal/enable");
    }

    void applyBalanced() override {
        applyPpmPolicies(1);

        FileIO::apply("1", "/sys/kernel/fpsgo/common/force_onoff");
        FileIO::apply("1", "/sys/kernel/fpsgo/fstb/adopt_low_fps");
        FileIO::apply("0", "/sys/pnpmgr/fpsgo_boost/boost_enable");
        FileIO::apply("0", "/sys/kernel/fpsgo/fstb/gpu_slowdown_check");
        FileIO::apply("1", "/sys/kernel/fpsgo/fstb/fstb_self_ctrl_fps_enable");
        FileIO::apply("0", "/sys/kernel/fpsgo/fbt/enable_switch_down_throttle");
        FileIO::apply("1", "/sys/module/mtk_fpsgo/parameters/perfmgr_enable");
        FileIO::apply("0", "/proc/cpufreq/cpufreq_cci_mode");
        FileIO::apply("2", "/proc/cpufreq/cpufreq_power_mode");
        FileIO::apply("0", "/proc/perfmgr/syslimiter/syslimiter_force_disable");
        FileIO::apply("0", "/sys/devices/platform/boot_dramboost/dramboost/dramboost");
        FileIO::apply("2", "/sys/devices/system/cpu/eas/enable");
        FileIO::apply("0", "/sys/module/ged/parameters/is_GED_KPI_enabled");

        FileIO::write("0",  "/proc/gpufreq/gpufreq_opp_freq");
        FileIO::write("-1", "/proc/gpufreqv2/fix_target_opp_index");
        FileIO::apply("1",  "/sys/module/ged/parameters/gpu_dvfs_enable");

        int minIdx = resolveGpuIndex(false);
        FileIO::applyLL(minIdx, "/sys/kernel/ged/hal/custom_boost_gpu_freq");

        FileIO::apply("ignore_batt_oc 0",         "/proc/gpufreq/gpufreq_power_limited");
        FileIO::apply("ignore_batt_percent 0",     "/proc/gpufreq/gpufreq_power_limited");
        FileIO::apply("ignore_low_batt 0",         "/proc/gpufreq/gpufreq_power_limited");
        FileIO::apply("ignore_thermal_protect 0",  "/proc/gpufreq/gpufreq_power_limited");
        FileIO::apply("ignore_pbm_limited 0",      "/proc/gpufreq/gpufreq_power_limited");
        FileIO::apply("stop 0", "/proc/mtk_batoc_throttling/battery_oc_protect_stop");

        FileIO::write("-1", "/sys/devices/platform/10012000.dvfsrc/helio-dvfsrc/dvfsrc_req_ddr_opp");
        FileIO::write("-1", "/sys/kernel/helio-dvfsrc/dvfsrc_force_vcore_dvfs_opp");
        DevFreq::unlock("/sys/class/devfreq/mtk-dvfsrc-devfreq");
        FileIO::apply("1", "/sys/kernel/eara_thermal/enable");
    }

    void applyEfficiency() override {
        FileIO::apply("1", "/sys/kernel/fpsgo/fstb/adopt_low_fps");
        FileIO::apply("0", "/sys/pnpmgr/fpsgo_boost/boost_enable");
        FileIO::apply("0", "/sys/kernel/fpsgo/fstb/gpu_slowdown_check");
        FileIO::apply("1", "/sys/kernel/fpsgo/fbt/enable_switch_down_throttle");
        FileIO::apply("1", "/sys/module/mtk_fpsgo/parameters/perfmgr_enable");
        FileIO::apply("0", "/proc/perfmgr/syslimiter/syslimiter_force_disable");
        FileIO::apply("0", "/sys/devices/platform/boot_dramboost/dramboost/dramboost");
        FileIO::apply("1", "/sys/devices/system/cpu/eas/enable");
        FileIO::apply("2", "/proc/cpufreq/cpufreq_cci_mode");
        FileIO::apply("1", "/proc/cpufreq/cpufreq_power_mode");

        applyGpuEfficiency();

        applyPpmEfficiency();
    }

private:
    const Config& cfg_;

    void applyPpmPolicies(int enable) noexcept {
        if (!FileIO::exists("/proc/ppm/policy_status")) return;
        FILE* fp = ::fopen("/proc/ppm/policy_status", "r");
        if (!fp) return;
        char line[1024];
        while (::fgets(line, sizeof(line), fp)) {
            if (!cfg_.ppmPolicy.empty() && ::strstr(line, cfg_.ppmPolicy.c_str())) {
                auto cmd = std::format("{} {}", line[1], enable);
                FileIO::apply(cmd, "/proc/ppm/policy_status");
            }
        }
        ::fclose(fp);
    }

    int resolveGpuIndex(bool mid) noexcept {
        if (FileIO::exists("/proc/gpufreqv2/gpu_working_opp_table"))
            return mid ? DevFreq::mtkGpuMidFreqIndex("/proc/gpufreqv2/gpu_working_opp_table")
                       : DevFreq::mtkGpuMinFreqIndex("/proc/gpufreqv2/gpu_working_opp_table");
        if (FileIO::exists("/proc/gpufreq/gpufreq_opp_dump"))
            return mid ? DevFreq::mtkGpuMidFreqIndex("/proc/gpufreq/gpufreq_opp_dump")
                       : DevFreq::mtkGpuMinFreqIndex("/proc/gpufreq/gpufreq_opp_dump");
        return 0;
    }

    long parseGpuFreqFromDump(bool getMax) noexcept {
        FILE* fp = ::fopen("/proc/gpufreq/gpufreq_opp_dump", "r");
        if (!fp) return 0;
        char line[1024];
        long target = getMax ? 0 : LONG_MAX;
        while (::fgets(line, sizeof(line), fp)) {
            char* fs = ::strstr(line, "freq =");
            if (!fs) continue;
            long f = ::atol(fs + 6);
            if (getMax && f > target) target = f;
            if (!getMax && f < target) target = f;
        }
        ::fclose(fp);
        return (target == LONG_MAX) ? 0 : target;
    }

    void applyGpuEsport() noexcept {
        if (!cfg_.liteMode) {
            if (FileIO::exists("/proc/gpufreqv2"))
                FileIO::apply("0", "/proc/gpufreqv2/fix_target_opp_index");
            else if (FileIO::exists("/proc/gpufreq/gpufreq_opp_dump")) {
                long f = parseGpuFreqFromDump(true);
                if (f) FileIO::applyLL(f, "/proc/gpufreq/gpufreq_opp_freq");
            }
        } else {
            FileIO::apply("0",  "/proc/gpufreq/gpufreq_opp_freq");
            FileIO::apply("-1", "/proc/gpufreqv2/fix_target_opp_index");
            int midIdx = resolveGpuIndex(true);
            FileIO::applyLL(midIdx, "/sys/kernel/ged/hal/custom_boost_gpu_freq");
        }
    }

    void applyGpuEfficiency() noexcept {
        if (FileIO::exists("/proc/gpufreqv2")) {
            int minIdx = DevFreq::mtkGpuMinFreqIndex("/proc/gpufreqv2/gpu_working_opp_table");
            FileIO::applyLL(minIdx, "/proc/gpufreqv2/fix_target_opp_index");
        } else if (FileIO::exists("/proc/gpufreq/gpufreq_opp_dump")) {
            long minFreq = parseGpuFreqFromDump(false);
            if (minFreq) FileIO::applyLL(minFreq, "/proc/gpufreq/gpufreq_opp_freq");
        }
    }

    void applyDramEsport() noexcept {
        if (!cfg_.liteMode) {
            FileIO::apply("0", "/sys/devices/platform/10012000.dvfsrc/helio-dvfsrc/dvfsrc_req_ddr_opp");
            FileIO::apply("0", "/sys/kernel/helio-dvfsrc/dvfsrc_force_vcore_dvfs_opp");
            DevFreq::maxPerf("/sys/class/devfreq/mtk-dvfsrc-devfreq");
        } else {
            FileIO::apply("-1", "/sys/devices/platform/10012000.dvfsrc/helio-dvfsrc/dvfsrc_req_ddr_opp");
            FileIO::apply("-1", "/sys/kernel/helio-dvfsrc/dvfsrc_force_vcore_dvfs_opp");
            DevFreq::midPerf("/sys/class/devfreq/mtk-dvfsrc-devfreq");
        }
    }

    void applyPpmEfficiency() noexcept {
        if (!FileIO::exists("/proc/ppm/policy/hard_userlimit_max_cpu_freq") &&
            !FileIO::exists("/proc/ppm/policy/hard_userlimit_min_cpu_freq")) return;

        DIR* dir = ::opendir("/sys/devices/system/cpu/cpufreq");
        if (!dir) return;
        struct dirent* e;
        int cluster = 0;
        while ((e = ::readdir(dir))) {
            if (!::strstr(e->d_name, "policy")) continue;
            auto maxInfoPath = std::format("/sys/devices/system/cpu/cpufreq/{}/cpuinfo_max_freq", e->d_name);
            auto minInfoPath = std::format("/sys/devices/system/cpu/cpufreq/{}/cpuinfo_min_freq", e->d_name);
            auto availPath   = std::format("/sys/devices/system/cpu/cpufreq/{}/scaling_available_frequencies", e->d_name);

            long cpuMax = FileIO::readInt(maxInfoPath);
            long cpuMin = FileIO::readInt(minInfoPath);
            long midFreq = CpuFreq::getMidFreq(availPath);
            if (midFreq <= 0) midFreq = cpuMin;

            if (FileIO::exists("/proc/ppm/policy/hard_userlimit_max_cpu_freq"))
                FileIO::apply(std::format("{} {}", cluster, cpuMax), "/proc/ppm/policy/hard_userlimit_max_cpu_freq");
            if (FileIO::exists("/proc/ppm/policy/hard_userlimit_min_cpu_freq"))
                FileIO::apply(std::format("{} {}", cluster, midFreq), "/proc/ppm/policy/hard_userlimit_min_cpu_freq");
            ++cluster;
        }
        ::closedir(dir);
    }
};

// Factory helper — registered in SocManager.cpp
std::unique_ptr<SocManager> makeMediaTek(const Config& cfg) {
    return std::make_unique<MediaTekManager>(cfg);
}

} // namespace velfox
