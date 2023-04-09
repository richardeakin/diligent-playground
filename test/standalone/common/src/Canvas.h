#pragma once

#include "BasicMath.hpp"
#include "RefCntAutoPtr.hpp"
#include "DeviceContext.h"
#include "Buffer.h"

namespace ju {

namespace dg = Diligent;

//! Renders a 2D Quad for misc. drawing
//! TODO: expose shader and constants buffer for reuse
class Canvas {
public:
	Canvas( const dg::int2 &size = { 1, 1 } );

	void setSize( const dg::int2 &size );

	void update( double deltaSeconds );
	void render( dg::IDeviceContext* context, const dg::float4x4 &mvp );

private:
	void initPipelineState();
	void watchShadersDir();
	void reloadOnAssetsUpdated();

	dg::int2 mSize;

	dg::RefCntAutoPtr<dg::IPipelineState>			mPSO;
	dg::RefCntAutoPtr<dg::IShaderResourceBinding>	mSRB;
	dg::RefCntAutoPtr<dg::IBuffer>					mShaderConstants;
};

} // namespace ju