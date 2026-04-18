#include "SocManager.hpp"
#include "FileIO.hpp"
#include "DevFreq.hpp"
#include <dirent.h>
#include <cstring>
#include <format>

namespace velfox {

class UnisocManager final : public SocManager {
public:
    explicit UnisocManager(const Config& cfg) : cfg_(cfg) {}

    void applyEsport() override {
        applyGpu([&](std::string_view p) {
            cfg_.liteMode ? DevFreq::midPerf(p) : DevFreq::maxPerf(p);
        });
    }

    void applyBalanced() override {
        applyGpu([](std::string_view p) { DevFreq::unlock(p); });
    }

    void applyEfficiency() override {
        applyGpu([](std::string_view p) { DevFreq::minPerf(p); });
    }

private:
    const Config& cfg_;

    template<typename Fn>
    static void applyGpu(Fn&& fn) noexcept {
        DIR* dir = ::opendir("/sys/class/devfreq");
        if (!dir) return;
        struct dirent* e;
        while ((e = ::readdir(dir))) {
            if (::strstr(e->d_name, ".gpu")) {
                fn(std::format("/sys/class/devfreq/{}", e->d_name));
                break;
            }
        }
        ::closedir(dir);
    }
};

std::unique_ptr<SocManager> makeUnisoc(const Config& cfg) {
    return std::make_unique<UnisocManager>(cfg);
}

} // namespace velfox
