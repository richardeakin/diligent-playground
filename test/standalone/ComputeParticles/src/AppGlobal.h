#pragma once

#include "RenderDevice.h"

namespace app {

namespace dg = Diligent;

// simple struct to pass around global data for creating things, that are owned by the main app.
struct AppGlobal {
		dg::IRenderDevice*						renderDevice = nullptr;
		dg::IShaderSourceInputStreamFactory*	shaderSourceFactory = nullptr;
		const dg::SwapChainDesc*				swapChainImageDesc = nullptr;
};

AppGlobal* global();

} // namespace app
