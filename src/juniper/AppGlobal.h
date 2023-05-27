#pragma once

#include "RenderDevice.h"
#include "Shader.h"
#include "RefCntAutoPtr.hpp"

#include <filesystem>

namespace juniper {

namespace dg = Diligent;
namespace fs = std::filesystem;

// simple struct to pass around global data for creating things, created by the main app but used in other files.
struct AppGlobal {
		dg::IRenderDevice*										renderDevice = nullptr;
		dg::RefCntAutoPtr<dg::IShaderSourceInputStreamFactory>	shaderSourceFactory;
		dg::TEXTURE_FORMAT										colorBufferFormat = dg::TEX_FORMAT_RGBA8_UNORM;
		dg::TEXTURE_FORMAT										depthBufferFormat = dg::TEX_FORMAT_UNKNOWN;

		fs::path				repoRootPath;
};

AppGlobal* global();

#if ! defined( CINDER_DISABLED )
//! Sets the path to the repo root on AppGlobal, by walking up from the executable path until we find \a rootFile
void findRepoRootPath( const fs::path &rootFile = ".git" );
//! Returns the path to the asset that lives within the root repo's assets folder
fs::path getRootAssetPath( const fs::path &relativePath );
#endif

} // namespace juniper

