#pragma once

#include <filesystem>

namespace juniper {

namespace fs = std::filesystem;

//! settingsFile is relative to lppPath
bool initLivePP( const fs::path &lppPath, const fs::path &settingsFile = "livepp.json" );

} // namespace juniper