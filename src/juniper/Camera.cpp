#include "Camera.h"

#include "cinder/Log.h"

#include "glm/gtc/quaternion.hpp"

using namespace glm;

namespace juniper {

// -------------------------------------------------------------------------------------------------------
// Camera
// -------------------------------------------------------------------------------------------------------

void Camera::lookAt( const vec3 &eyeOrigin, const vec3 &eyeTarget, const vec3 &worldUp )
{
	mEyeOrigin = eyeOrigin;
	mEyeTarget = eyeTarget;
	mWorldUp = worldUp; // TODO: remove this and use up extracted from view matrix

	mViewMatrix = glm::lookAt( mEyeOrigin, mEyeTarget, mWorldUp );

	auto viewDir = normalize( mEyeTarget - mEyeOrigin );
	//mOrientation = glm::rotation( viewDir, glm::vec3( 0, 0, -1 ) ); // TODO: should this be pointing at positive z?
	mOrientation = glm::rotation( viewDir, glm::vec3( 0, 0, 1 ) );
}

void Camera::perspective( float fov, float aspectRatio, float nearClip, float farClip )
{
	mFov = fov;
	mAspectRatio = aspectRatio;
	mNearClip = nearClip;
	mFarClip = farClip;

	mProjectionMatrix = glm::perspective( mFov, mAspectRatio, mNearClip, mFarClip );
}

void Camera::setFov( float verticalFov )
{ 
	mFov = verticalFov;
	mProjectionMatrix = glm::perspective( mFov, mAspectRatio, mNearClip, mFarClip );
}

void Camera::setOrientation( const quat &orientation )
{
	mOrientation = normalize( orientation );
	vec3 right   = rotate( mOrientation, vec3( 1, 0, 0 ) );
	vec3 up		 = rotate( mOrientation, vec3( 0, 1, 0 ) );
	vec3 forward = rotate( mOrientation, vec3( 0, 0, 1 ) );
	vec3 d		 = { - dot( mEyeOrigin, right ), - dot( mEyeOrigin, up ), - dot( mEyeOrigin, forward ) };

	// TODO: check this order
	mViewMatrix = {
		right.x,   right.y,   right.z,   d.x,
		up.x,	   up.y,	  up.z,      d.y,
		forward.x, forward.y, forward.z, d.z,
		0,		   0,         0,         1
	};
}

vec3 Camera::getWorldRight() const
{
	return vec3( row( getViewMatrix(), 0 ) );
}

vec3 Camera::getWorldUp() const
{
	return vec3( row( getViewMatrix(), 1 ) );
}

vec3 Camera::getWorldForward() const
{
	return vec3( row( getViewMatrix(), 2 ) );
}

// -------------------------------------------------------------------------------------------------------
// FlyCam
// -------------------------------------------------------------------------------------------------------

void FlyCam::perspective( float fov, const vec2 &windowSize, float nearClip, float farClip )
{
	mWindowSize = windowSize;
	Camera::perspective( mFov, mWindowSize.x / mWindowSize.y, mNearClip, mFarClip );
}

void FlyCam::setWindowSize( const vec2 &windowSize )
{
	mWindowSize = windowSize;
	Camera::perspective( mFov, mWindowSize.x / mWindowSize.y, mNearClip, mFarClip );
}

void FlyCam::mouseEvent( MouseEvent &e )
{
	if( ! mEnabled )
		return;

	bool handled = true;
	switch( e.getState() ) {
		case MouseEvent::State::Press:
			mouseDown( e.getPos() );
			break;
		case MouseEvent::State::Release:
			mouseUp( e.getPos() );
			break;
		case MouseEvent::State::Move:
			if( e.isDrag() ) {
				mouseDrag( e.getPos() );
			}
			break;
		case MouseEvent::State::Scroll:
			mouseWheel( e.getScroll().x );
			break;
	}

	e.setHandled();
}

void FlyCam::mouseDown( const vec2 &mousePos )
{
	if( ! mEnabled )
		return;

	mMouseDown = true;
	mInitialMousePos = mousePos;
	mOrientationMouseDown = mOrientation;
}

void FlyCam::mouseUp( const vec2 &mousePos )
{
	if( ! mEnabled )
		return;

	mMouseDown = false;
	mLookDelta = vec2( 0 );
	//mInitialCam = *mCamera;
}

// TODO: expose these magic numbers?

//void FlyCam::mouseDrag( const vec2 &mousePos, bool leftDown, bool middleDown, bool rightDown )
void FlyCam::mouseDrag( const vec2 &mousePos )
{
	if( ! mEnabled )
		return;

	mLookDelta = ( mousePos - mInitialMousePos ) / mWindowSize;
	mLookDelta *= 2.75f;
}

void FlyCam::mouseWheel( float increment )
{
	if( ! mEnabled )
		return;

	mMoveAccel.y = mMoveSpeed * increment * 7.0f; // TODO: expse multiplier as param
}

void FlyCam::keyDown( KeyEvent &event )
{
	if( ! mEnabled )
		return;

	// skip if any modifier key is being pressed, except shift which is used to decrease speed.
	if( event.isAltDown() || event.isControlDown() || event.isMetaDown() )
		return;

	bool handled = true;
	float moveAmount = mMoveSpeed;
	if( event.isShiftDown() )
		moveAmount *= 0.1f;

	const char c = tolower( event.getChar() );

	if( c == 'a' || event.getKey() == KeyEvent::Key::KEY_LEFT ) {
		mMoveDirection.x = - moveAmount;
	}
	else if( c == 'd' || event.getKey() == KeyEvent::Key::KEY_RIGHT ) {
		mMoveDirection.x = moveAmount;
	}
	else if( c == 'w' ) {
		mMoveDirection.y = moveAmount;
	}
	else if( c == 's' ) {
		mMoveDirection.y = - moveAmount;
	}
	else if( c == 'e' || event.getKey() == KeyEvent::Key::KEY_UP ) {
		mMoveDirection.z = moveAmount;
	}
	else if( c == 'c' || event.getKey() == KeyEvent::Key::KEY_DOWN ) {
		mMoveDirection.z = - moveAmount;
	}
	else
		handled = false;

	event.setHandled( handled );
}

void FlyCam::keyUp( KeyEvent &event )
{
	bool handled = true;
	const char c = tolower( event.getChar() );
	if( c == 'a' || c == 'd' || event.getKey() == KeyEvent::Key::KEY_LEFT || event.getKey() == KeyEvent::Key::KEY_RIGHT ) {
		mMoveDirection.x = 0;
		mMoveAccel.x = 0;
	}
	else if( c == 'w' || c == 's' ) {
		mMoveDirection.y = 0;
		mMoveAccel.y = 0;
	}
	else if( c == 'e' || c == 'c' || event.getKey() == KeyEvent::Key::KEY_UP || event.getKey() == KeyEvent::Key::KEY_DOWN ) {
		mMoveDirection.z = 0;
		mMoveAccel.z = 0;
	}
	else
		handled = false;

	event.setHandled( handled );
}

void FlyCam::stop()
{
	mMoveDirection = vec3( 0 );
	mMoveAccel = vec3( 0 );
	mMoveVelocity = vec3( 0 );
}

void FlyCam::update()
{
	if( mMouseDown ) {
		quat orientation = mOrientationMouseDown;
		orientation = normalize( orientation * angleAxis( mLookDelta.x, vec3( 0, -1, 0 ) ) );
		orientation = normalize( orientation * angleAxis( mLookDelta.y, vec3( -1, 0, 0 ) ) );

		setOrientation( orientation );
		//setWorldUp( vec3( 0, 1, 0 ) );
	}

	mMoveAccel += mMoveDirection;

	const float maxVelocity = mMoveSpeed * 5;

	vec3 targetVelocity = glm::clamp( mMoveAccel * 0.3f, vec3( -maxVelocity ), vec3( maxVelocity ) );
	mMoveVelocity = lerp( mMoveVelocity, targetVelocity, 0.3f );

	if( glm::length( mMoveVelocity ) > 0.01 ) {
		CI_LOG_I( "mMoveDirection: " << mMoveDirection << ", mMoveAccel: " << mMoveAccel << ", mMoveVelocity: " << mMoveVelocity );
	}

	if( glm::length( mLookDelta ) > 0.01 ) {
		CI_LOG_I( "mLookDelta: " << mLookDelta );
	}

	vec3 eye = getEyeOrigin();
	vec3 up = vec3( 0, 1, 0 );
	eye += getWorldRight() * mMoveVelocity.x;
	eye += getWorldForward() * mMoveVelocity.y;
	eye += up * mMoveVelocity.z;

	// TODO: does eye target need to be adjusted? should perhaps not be calling lookAt again here
	//mCamera->setEyePoint( eye );
	lookAt( eye, mEyeTarget, up );

	const float drag = 0.2f;
	mMoveAccel *= 1 - drag;
}

} // namespace juniper