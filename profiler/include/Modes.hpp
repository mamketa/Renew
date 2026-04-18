#pragma once
#include "Common.hpp"
#include "SocManager.hpp"

namespace velfox {

Config readConfig() noexcept;
void perfCommon() noexcept;
void esportMode(const Config& cfg) noexcept;
void balancedMode(const Config& cfg) noexcept;
void efficiencyMode(const Config& cfg) noexcept;
void setRpsXps(int mode) noexcept;
void setDnd(int mode) noexcept;

} // namespace velfox
