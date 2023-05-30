#include "FlyCam.h"

namespace juniper {

// -------------------------------------------------------------------------------------------------------
// Camera
// -------------------------------------------------------------------------------------------------------

void Camera::lookAt( const vec3 &eyeOrigin, const vec3 &eyeTarget, const vec3 &worldUp )
{
	mEyeOrigin = eyeOrigin;
	mEyeTarget = eyeTarget;
	mWorldUp = worldUp;

	mViewMatrix = glm::lookAt( mEyeOrigin, mEyeTarget, mWorldUp );
}

void Camera::perspective( float fov, float aspectRatio, float nearClip, float farClip )
{
	mFov = fov;
	mAspectRatio = aspectRatio;
	mNearClip = nearClip;
	farClip = farClip;

	mProjectionMatrix = glm::perspective( mFov, mAspectRatio, mNearClip, mFarClip );
}

// -------------------------------------------------------------------------------------------------------
// FlyCam
// -------------------------------------------------------------------------------------------------------

void FlyCam::mouseDown( MouseEvent &event )
{
	if( ! mEnabled )
		return;

	mouseDown( event.getPos() );
	event.setHandled();
}

void FlyCam::mouseUp( MouseEvent &event )
{
}

void FlyCam::mouseWheel( MouseEvent &event )
{
}

void FlyCam::mouseDrag( MouseEvent &event )
{
}

void FlyCam::keyDown( KeyEvent &event )
{
}

void FlyCam::keyUp( KeyEvent &event )
{
}

void FlyCam::mouseDown( const vec2 &mousePos )
{
}

void FlyCam::mouseUp( const ci::vec2 &mousePos )
{
}

void FlyCam::mouseWheel( float increment )
{
}

void FlyCam::mouseDrag( const ci::vec2 &mousePos, bool leftDown, bool middleDown, bool rightDown )
{
}

void FlyCam::update()
{
}

} // namespace juniper