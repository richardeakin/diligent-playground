#pragma once

#include <filesystem>

namespace ju {

namespace fs = std::filesystem;

bool initLivePP( const fs::path &lppPath );
//bool initLivePP( const std::filesystem::path &lppPath );

} // namespace ju