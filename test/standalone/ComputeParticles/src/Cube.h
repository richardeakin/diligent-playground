
#pragma once

#include "RenderDevice.h"
#include "RefCntAutoPtr.hpp"
#include "Buffer.h"
#include "Image.h"

namespace ju {

namespace dg = Diligent;

class Cube {
public:
	//! Things owned by the main app
	struct AppCreateInfo {
		dg::IRenderDevice*						renderDevice = nullptr;
		dg::IShaderSourceInputStreamFactory*	shaderSourceFactory = nullptr;
		const dg::SwapChainDesc*				swapChainImageDesc = nullptr;
	};

	Cube( const AppCreateInfo &createInfo );

	void render();

private:
	void initPipelineState( const AppCreateInfo &createInfo );
	void initVertexBuffer( const AppCreateInfo &createInfo );
	void initIndexBuffer( const AppCreateInfo &createInfo );

	dg::RefCntAutoPtr<dg::IPipelineState>         m_pPSO;
	dg::RefCntAutoPtr<dg::IBuffer>                m_CubeVertexBuffer;
	dg::RefCntAutoPtr<dg::IBuffer>                m_CubeIndexBuffer;
	dg::RefCntAutoPtr<dg::IBuffer>                m_VSConstants;
	dg::RefCntAutoPtr<dg::IShaderResourceBinding> m_SRB;
};

} // namespace ju