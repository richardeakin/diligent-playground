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
};

AppGlobal* global();

const fs::path& getRepoRootPath( const fs::path &rootFile = ".git" );


} // namespace juniper

