// Todo: Make this a CubeTest for Cinder-Gx (currently using gl)

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;

class RotatingCubeApp : public App {
  public:	
	void	setup() override;
	void	resize() override;
	void	update() override;
	void	draw() override;
	
	CameraPersp			mCam;
};

void RotatingCubeApp::setup()
{
	mCam.lookAt( vec3( 3, 2, 4 ), vec3( 0 ) );
}

void RotatingCubeApp::resize()
{
	mCam.setPerspective( 60, getWindowAspectRatio(), 1, 1000 );
}

void RotatingCubeApp::update()
{
}

void RotatingCubeApp::draw()
{
}

CINDER_APP( RotatingCubeApp, RendererGl( RendererGl::Options().msaa( 16 ) ) )
