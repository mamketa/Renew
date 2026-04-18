#include "Modes.hpp"
#include <print>
#include <cstdlib>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::println("Usage: {} <mode>", argv[0]);
        std::println("  0 - Common tweaks");
        std::println("  1 - Esport mode");
        std::println("  2 - Balanced mode");
        std::println("  3 - Efficiency mode");
        return 1;
    }

    int mode = std::atoi(argv[1]);
    auto cfg = velfox::readConfig();

    switch (mode) {
        case 0: velfox::perfCommon(); break;
        case 1: velfox::perfCommon(); velfox::esportMode(cfg);    break;
        case 2: velfox::perfCommon(); velfox::balancedMode(cfg);   break;
        case 3: velfox::perfCommon(); velfox::efficiencyMode(cfg); break;
        default:
            std::println("Invalid mode: {}", mode);
            return 1;
    }
    return 0;
}
