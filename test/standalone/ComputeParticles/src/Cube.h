
#pragma once

//#include "RenderDevice.h"
#include "RefCntAutoPtr.hpp"
#include "Buffer.h"
#include "Image.h"
#include "DeviceContext.h"
#include "BasicMath.hpp"

namespace ju {

namespace dg = Diligent;

enum VERTEX_COMPONENT_FLAGS : int
{
	VERTEX_COMPONENT_FLAG_NONE     = 0x00,
	VERTEX_COMPONENT_FLAG_POSITION = 0x01,
	VERTEX_COMPONENT_FLAG_NORMAL   = 0x02,
	VERTEX_COMPONENT_FLAG_TEXCOORD = 0x04,

	VERTEX_COMPONENT_FLAG_POS_UV =
	VERTEX_COMPONENT_FLAG_POSITION |
	VERTEX_COMPONENT_FLAG_TEXCOORD,

	VERTEX_COMPONENT_FLAG_POS_NORM_UV =
	VERTEX_COMPONENT_FLAG_POSITION |
	VERTEX_COMPONENT_FLAG_NORMAL |
	VERTEX_COMPONENT_FLAG_TEXCOORD
};
DEFINE_FLAG_ENUM_OPERATORS(VERTEX_COMPONENT_FLAGS);

class Cube {
public:

	Cube( VERTEX_COMPONENT_FLAGS components = VERTEX_COMPONENT_FLAG_POS_UV );

	void update( double deltaSeconds );
	void render( dg::IDeviceContext* context, const dg::float4x4 &mvp );

	void setTransform( const dg::float4x4 &m )	{ mTransform = m; }

	void setLightDir( const dg::float3 &dir )	{ mLightDirection = dir; }

private:
	void initPipelineState();
	void initVertexBuffer();
	void initIndexBuffer();

	void watchShadersDir();
	void reloadOnAssetsUpdated();

	dg::RefCntAutoPtr<dg::IPipelineState>         m_pPSO;
	dg::RefCntAutoPtr<dg::IBuffer>                m_CubeVertexBuffer;
	dg::RefCntAutoPtr<dg::IBuffer>                m_CubeIndexBuffer;
	dg::RefCntAutoPtr<dg::IBuffer>                m_VSConstants;
	dg::RefCntAutoPtr<dg::IShaderResourceBinding> m_SRB;

	VERTEX_COMPONENT_FLAGS mComponents;

	dg::float3      mLightDirection  = dg::float3(0, 1, 0); // TODO: this should be part of a global constants buffer
	dg::float4x4	mTransform;
};

} // namespace ju