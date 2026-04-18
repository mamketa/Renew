#include "SocManager.hpp"
#include <print>

namespace velfox {

extern std::unique_ptr<SocManager> makeMediaTek(const Config&);
extern std::unique_ptr<SocManager> makeSnapdragon(const Config&);
extern std::unique_ptr<SocManager> makeExynos(const Config&);
extern std::unique_ptr<SocManager> makeUnisoc(const Config&);
extern std::unique_ptr<SocManager> makeTensor(const Config&);

std::unique_ptr<SocManager> SocManager::create(const Config& cfg) {
    switch (cfg.soc) {
        case 1: std::println("SoC: MediaTek");   return makeMediaTek(cfg);
        case 2: std::println("SoC: Snapdragon"); return makeSnapdragon(cfg);
        case 3: std::println("SoC: Exynos");     return makeExynos(cfg);
        case 4: std::println("SoC: Unisoc");     return makeUnisoc(cfg);
        case 5: std::println("SoC: Tensor");     return makeTensor(cfg);
        default:
            std::println("SoC: unknown ({}), skipping SoC-specific tweaks", cfg.soc);
            return nullptr;
    }
}

} // namespace velfox
