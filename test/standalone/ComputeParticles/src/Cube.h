
#pragma once

//#include "RenderDevice.h"
#include "RefCntAutoPtr.hpp"
#include "Buffer.h"
#include "Image.h"
#include "DeviceContext.h"
#include "BasicMath.hpp"

namespace ju {

namespace dg = Diligent;

class Cube {
public:
	Cube();

	void update( double deltaSeconds );
	void render( dg::IDeviceContext* context, const dg::float4x4 &mvp );

private:
	void initPipelineState();
	void initVertexBuffer();
	void initIndexBuffer();

	void reloadOnAssetsUpdated();

	dg::RefCntAutoPtr<dg::IPipelineState>         m_pPSO;
	dg::RefCntAutoPtr<dg::IBuffer>                m_CubeVertexBuffer;
	dg::RefCntAutoPtr<dg::IBuffer>                m_CubeIndexBuffer;
	dg::RefCntAutoPtr<dg::IBuffer>                m_VSConstants;
	dg::RefCntAutoPtr<dg::IShaderResourceBinding> m_SRB;

	dg::float4x4	mTransform; // TODO: use this to rotate the cube from main app
};

} // namespace ju