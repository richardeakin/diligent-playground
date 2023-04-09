#include "Canvas.h"

using namespace ju;
using namespace Diligent;

Canvas::Canvas( const dg::int2 &size )
	: mSize( size )
{
	initPipelineState();
	watchShadersDir();
}

void Canvas::setSize( const int2 &size )
{
	if( mSize == size ) {
		return;
	}
	mSize = size;
}

void Canvas::initPipelineState()
{
}

void Canvas::watchShadersDir()
{
}

void Canvas::reloadOnAssetsUpdated()
{
}

void Canvas::update( double deltaSeconds )
{
}

void Canvas::render( IDeviceContext* context, const float4x4 &mvp )
{
}
