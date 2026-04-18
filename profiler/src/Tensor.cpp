#include "SocManager.hpp"
#include "FileIO.hpp"
#include "CpuFreq.hpp"
#include "DevFreq.hpp"
#include <dirent.h>
#include <cstring>
#include <format>

namespace velfox {

class TensorManager final : public SocManager {
public:
    explicit TensorManager(const Config& cfg) : cfg_(cfg) {}

    void applyEsport() override {
        applyMaliGpu([&](std::string_view avail, std::string_view maxF, std::string_view minF) {
            long mx = CpuFreq::getMaxFreq(avail);
            FileIO::applyLL(mx, maxF);
            long target = cfg_.liteMode ? CpuFreq::getMidFreq(avail) : mx;
            FileIO::applyLL(target, minF);
        });
        if (!cfg_.deviceMitigation)
            applyMifDevfreq([&](std::string_view p) {
                cfg_.liteMode ? DevFreq::midPerf(p) : DevFreq::maxPerf(p);
            });
    }

    void applyBalanced() override {
        applyMaliGpu([](std::string_view avail, std::string_view maxF, std::string_view minF) {
            FileIO::writeLL(CpuFreq::getMaxFreq(avail), maxF);
            FileIO::writeLL(CpuFreq::getMinFreq(avail), minF);
        });
        if (!cfg_.deviceMitigation)
            applyMifDevfreq([](std::string_view p) { DevFreq::unlock(p); });
    }

    void applyEfficiency() override {
        applyMaliGpu([](std::string_view avail, std::string_view maxF, std::string_view minF) {
            long mn = CpuFreq::getMinFreq(avail);
            FileIO::applyLL(mn, minF);
            FileIO::applyLL(mn, maxF);
        });
    }

private:
    const Config& cfg_;

    template<typename Fn>
    static void applyMaliGpu(Fn&& fn) noexcept {
        DIR* dir = ::opendir("/sys/devices/platform");
        if (!dir) return;
        struct dirent* e;
        while ((e = ::readdir(dir))) {
            if (::strstr(e->d_name, ".mali")) {
                auto base  = std::format("/sys/devices/platform/{}", e->d_name);
                auto avail = std::format("{}/available_frequencies", base);
                if (FileIO::exists(avail)) {
                    fn(avail,
                       std::format("{}/scaling_max_freq", base),
                       std::format("{}/scaling_min_freq", base));
                }
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

std::unique_ptr<SocManager> makeTensor(const Config& cfg) {
    return std::make_unique<TensorManager>(cfg);
}

} // namespace velfox
