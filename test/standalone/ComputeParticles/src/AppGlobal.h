#pragma once

#include "RenderDevice.h"
#include "Shader.h"
#include "RefCntAutoPtr.hpp"

namespace app {

namespace dg = Diligent;

// simple struct to pass around global data for creating things, created by the main app but used in other files.
struct AppGlobal {
		dg::IRenderDevice*										renderDevice = nullptr;
		dg::RefCntAutoPtr<dg::IShaderSourceInputStreamFactory>	shaderSourceFactory;
		const dg::SwapChainDesc*								swapChainImageDesc = nullptr;
};

AppGlobal* global();

} // namespace app
