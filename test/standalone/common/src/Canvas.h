#pragma once

#include "BasicMath.hpp"
#include "RefCntAutoPtr.hpp"
#include "DeviceContext.h"
#include "Buffer.h"

namespace ju {

namespace dg = Diligent;

//! Renders a 2D Quad for misc. drawing
// TODO: expose shader and constants buffer for reuse
// TODO: define public properties in pixel coordinates (currently in NDC
class Canvas {
public:
	Canvas( const dg::float2 &center = { 0, 0 }, const dg::float2 &size = { 1, 1 } );

	void				setCenter( const dg::float2 &center )	{ mCenter = center; }
	const dg::float2&	getCenter() const						{ return mCenter; }

	void				setSize( const dg::float2 &size )		{ mSize = size; }
	const dg::float2&	getSize() const							{ return mSize; }

	void update( double deltaSeconds );
	void render( dg::IDeviceContext* context, const dg::float4x4 &mvp );

private:
	void initPipelineState();
	void watchShadersDir();
	void reloadOnAssetsUpdated();

	dg::float2 mCenter;
	dg::float2 mSize;

	dg::RefCntAutoPtr<dg::IPipelineState>			mPSO;
	dg::RefCntAutoPtr<dg::IShaderResourceBinding>	mSRB;
	dg::RefCntAutoPtr<dg::IBuffer>					mShaderConstants;
};

} // namespace ju