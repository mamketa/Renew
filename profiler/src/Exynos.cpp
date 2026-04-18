#include "SocManager.hpp"
#include "FileIO.hpp"
#include "CpuFreq.hpp"
#include "DevFreq.hpp"
#include <dirent.h>
#include <cstring>
#include <format>

namespace velfox {

class ExynosManager final : public SocManager {
public:
    explicit ExynosManager(const Config& cfg) : cfg_(cfg) {}

    void applyEsport() override {
        applyGpu([&](std::string_view avail, std::string_view maxClock, std::string_view minClock) {
            long mx = CpuFreq::getMaxFreq(avail);
            FileIO::applyLL(mx, maxClock);
            long target = cfg_.liteMode ? CpuFreq::getMidFreq(avail) : mx;
            FileIO::applyLL(target, minClock);
        });
        setMaliPolicy("always_on");
        if (!cfg_.deviceMitigation)
            applyMifDevfreq([&](std::string_view p) {
                cfg_.liteMode ? DevFreq::midPerf(p) : DevFreq::maxPerf(p);
            });
    }

    void applyBalanced() override {
        applyGpu([](std::string_view avail, std::string_view maxClock, std::string_view minClock) {
            FileIO::writeLL(CpuFreq::getMaxFreq(avail), maxClock);
            FileIO::writeLL(CpuFreq::getMinFreq(avail), minClock);
        });
        setMaliPolicy("coarse_demand");
        if (!cfg_.deviceMitigation)
            applyMifDevfreq([](std::string_view p) { DevFreq::unlock(p); });
    }

    void applyEfficiency() override {
        applyGpu([](std::string_view avail, std::string_view maxClock, std::string_view minClock) {
            long mn = CpuFreq::getMinFreq(avail);
            FileIO::applyLL(mn, minClock);
            FileIO::applyLL(mn, maxClock);
        });
    }

private:
    const Config& cfg_;

    template<typename Fn>
    static void applyGpu(Fn&& fn) noexcept {
        constexpr std::string_view base = "/sys/kernel/gpu";
        if (!FileIO::exists(base)) return;
        auto avail    = std::format("{}/gpu_available_frequencies", base);
        auto maxClock = std::format("{}/gpu_max_clock", base);
        auto minClock = std::format("{}/gpu_min_clock", base);
        fn(avail, maxClock, minClock);
    }

    static void setMaliPolicy(std::string_view policy) noexcept {
        DIR* dir = ::opendir("/sys/devices/platform");
        if (!dir) return;
        struct dirent* e;
        while ((e = ::readdir(dir))) {
            if (::strstr(e->d_name, ".mali")) {
                FileIO::apply(policy, std::format("/sys/devices/platform/{}/power_policy", e->d_name));
                break;
            }
        }
        ::closedir(dir);
    }

    template<typename Fn>
    static void applyMifDevfreq(Fn&& fn) noexcept {
        DIR* dir = ::opendir("/sys/class/devfreq");
        if (!dir) return;
        struct dirent* e;
        while ((e = ::readdir(dir)))
            if (::strstr(e->d_name, "devfreq_mif"))
                fn(std::format("/sys/class/devfreq/{}", e->d_name));
        ::closedir(dir);
    }
};

std::unique_ptr<SocManager> makeExynos(const Config& cfg) {
    return std::make_unique<ExynosManager>(cfg);
}

} // namespace velfox
