#include "SocManager.hpp"
#include "FileIO.hpp"
#include "DevFreq.hpp"
#include <dirent.h>
#include <cstring>
#include <format>

namespace velfox {

class SnapdragonManager final : public SocManager {
public:
    explicit SnapdragonManager(const Config& cfg) : cfg_(cfg) {}

    void applyEsport() override {
        if (!cfg_.deviceMitigation) {
            applyBusDram([&](std::string_view p) {
                cfg_.liteMode ? DevFreq::midPerf(p) : DevFreq::maxPerf(p);
            });
            for (auto* comp : {"DDR","LLCC","L3"}) {
                auto p = std::format("/sys/devices/system/cpu/bus_dcvs/{}", comp);
                cfg_.liteMode ? DevFreq::qcomMidPerf(p) : DevFreq::qcomMaxPerf(p);
            }
        }
        cfg_.liteMode
            ? DevFreq::midPerf("/sys/class/kgsl/kgsl-3d0/devfreq")
            : DevFreq::maxPerf("/sys/class/kgsl/kgsl-3d0/devfreq");

        FileIO::apply("0", "/sys/class/kgsl/kgsl-3d0/throttling");
        FileIO::apply("0", "/sys/class/kgsl/kgsl-3d0/thermal_pwrlevel");
        FileIO::apply("1", "/sys/class/kgsl/kgsl-3d0/devfreq/adrenoboost");
        FileIO::apply("1", "/sys/class/kgsl/kgsl-3d0/force_clk_on");
        FileIO::apply("1", "/sys/class/kgsl/kgsl-3d0/force_no_nap");
        FileIO::apply("0", "/sys/class/kgsl/kgsl-3d0/bus_split");
        FileIO::apply("0", "/sys/class/kgsl/kgsl-3d0/perfcounter");
    }

    void applyBalanced() override {
        if (!cfg_.deviceMitigation) {
            applyBusDram([](std::string_view p) { DevFreq::unlock(p); });
            for (auto* comp : {"DDR","LLCC","L3"}) {
                auto p = std::format("/sys/devices/system/cpu/bus_dcvs/{}", comp);
                DevFreq::qcomUnlock(p);
            }
        }
        DevFreq::unlock("/sys/class/kgsl/kgsl-3d0/devfreq");
        FileIO::apply("1", "/sys/class/kgsl/kgsl-3d0/throttling");
        FileIO::apply("1", "/sys/class/kgsl/kgsl-3d0/thermal_pwrlevel");
        FileIO::apply("0", "/sys/class/kgsl/kgsl-3d0/devfreq/adrenoboost");
        FileIO::apply("0", "/sys/class/kgsl/kgsl-3d0/force_clk_on");
        FileIO::apply("0", "/sys/class/kgsl/kgsl-3d0/force_no_nap");
        FileIO::apply("1", "/sys/class/kgsl/kgsl-3d0/bus_split");
        FileIO::apply("0", "/sys/class/kgsl/kgsl-3d0/perfcounter");
    }

    void applyEfficiency() override {
        DevFreq::minPerf("/sys/class/kgsl/kgsl-3d0/devfreq");
        FileIO::apply("1", "/sys/class/kgsl/kgsl-3d0/throttling");
        FileIO::apply("1", "/sys/class/kgsl/kgsl-3d0/thermal_pwrlevel");
        FileIO::apply("0", "/sys/class/kgsl/kgsl-3d0/devfreq/adrenoboost");
        FileIO::apply("0", "/sys/class/kgsl/kgsl-3d0/force_clk_on");
        FileIO::apply("0", "/sys/class/kgsl/kgsl-3d0/force_no_nap");
        FileIO::apply("1", "/sys/class/kgsl/kgsl-3d0/bus_split");
        FileIO::apply("0", "/sys/class/kgsl/kgsl-3d0/perfcounter");
    }

private:
    const Config& cfg_;

    template<typename Fn>
    static void applyBusDram(Fn&& fn) noexcept {
        DIR* dir = ::opendir("/sys/class/devfreq");
        if (!dir) return;
        struct dirent* e;
        while ((e = ::readdir(dir))) {
            const char* n = e->d_name;
            if (::strstr(n,"cpu-lat")||::strstr(n,"cpu-bw")||::strstr(n,"llccbw")||
                ::strstr(n,"bus_llcc")||::strstr(n,"bus_ddr")||::strstr(n,"memlat")||
                ::strstr(n,"cpubw")||::strstr(n,"kgsl-ddr-qos")) {
                fn(std::format("/sys/class/devfreq/{}", n));
            }
        }
        ::closedir(dir);
    }
};

std::unique_ptr<SocManager> makeSnapdragon(const Config& cfg) {
    return std::make_unique<SnapdragonManager>(cfg);
}

} // namespace velfox
