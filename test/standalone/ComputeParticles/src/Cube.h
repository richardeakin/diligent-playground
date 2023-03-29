
#pragma once

#include "RenderDevice.h"
#include "RefCntAutoPtr.hpp"
#include "Buffer.h"
#include "Image.h"
#include "DeviceContext.h"
#include "BasicMath.hpp"

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

	void render( dg::IDeviceContext* context, const dg::float4x4 &mvp );

private:
	void initPipelineState( const AppCreateInfo &createInfo );
	void initVertexBuffer( const AppCreateInfo &createInfo );
	void initIndexBuffer( const AppCreateInfo &createInfo );

	dg::RefCntAutoPtr<dg::IPipelineState>         m_pPSO;
	dg::RefCntAutoPtr<dg::IBuffer>                m_CubeVertexBuffer;
	dg::RefCntAutoPtr<dg::IBuffer>                m_CubeIndexBuffer;
	dg::RefCntAutoPtr<dg::IBuffer>                m_VSConstants;
	dg::RefCntAutoPtr<dg::IShaderResourceBinding> m_SRB;

	dg::float4x4	mTransform; // TODO: use this to rotate the cube from main app
};

} // namespace ju