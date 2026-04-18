#pragma once
#include "Common.hpp"
#include <memory>

namespace velfox {

struct Config {
    int soc{0};
    int liteMode{0};
    int deviceMitigation{0};
    std::string defaultCpuGov{"schedutil"};
    std::string ppmPolicy;
};

class SocManager {
public:
    virtual ~SocManager() = default;
    virtual void applyEsport() = 0;
    virtual void applyBalanced() = 0;
    virtual void applyEfficiency() = 0;

    static std::unique_ptr<SocManager> create(const Config& cfg);
};

} // namespace velfox
