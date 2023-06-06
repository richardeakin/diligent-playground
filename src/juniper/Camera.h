#pragma once

#include "cinder/Matrix.h"
#include "cinder/Quaternion.h"
#include "juniper/AppEvents.h"

namespace juniper {

using glm::vec2;
using glm::vec3;
using glm::mat4;
using glm::quat;

//! General purpose camera class.
class Camera {
public:

	void lookAt( const vec3 &eyeOrigin, const vec3 &eyeTarget, const vec3 &worldUp = { 0, 1, 0 } );

	void perspective( float fov, float aspectRatio, float nearClip, float farClip );

	// TODO: add ortho()

	const vec3& getEyeOrigin() const { return mEyeOrigin; }
	const vec3& getEyeTarget() const { return mEyeTarget; }

	//! Returns the camera's vertical field of view measured in degrees.
	float	getFov() const { return mFov; }
	//! Sets the camera's vertical field of view measured in degrees.
	void	setFov( float verticalFov );

	const mat4& getViewMatrix() const		{ return mViewMatrix; }
	const mat4& getProjectionMatrix() const	{ return mProjectionMatrix; }

	vec3 getWorldRight() const;
	vec3 getWorldUp() const;
	vec3 getWorldForward() const;

	void setOrientation( const quat &orientation );
	const quat& getOrientation() const		{ return mOrientation; }

protected:
	vec3 mEyeOrigin = { 0, 0, -5 };
	vec3 mEyeTarget = { 0, 0, 0 };
	vec3 mWorldUp = { 0, 1, 0 };

	float mFov = 40; // sample code is using a value of fovy = 0.7854
	float mAspectRatio = 1.0f;
	float mNearClip = 0.1f;
	float mFarClip = 100.0f;

	// TODO: use bools to minimize number of calcs
	//bool mProjectionCached = false;
	//bool mViewCached = false;
	mat4 mViewMatrix;
	mat4 mProjectionMatrix;
	quat mOrientation;
};

//! Controls a camera and processes AppEvents for looking around in a first-person style of movement.
class FlyCam : public Camera {
public:

	void perspective( float fov, const vec2 &windowSize, float nearClip, float farClip );
	void setWindowSize( const vec2 &windowSize );

	//! Sets the amount of movement when a hotkey is pressed. Default is 1. Shift makes it 1/10th the movement
	void	setMoveSpeed( float speed )	{ mMoveSpeed = speed; }
	//! Sets the amount of movement when a hotkey is pressed. Default is 1. Shift makes it 1/10th the movement
	float	getMoveSpeed() const		{ return mMoveSpeed;}

	// TODO: combine these into one method that calls other non-event methods
	//void mouseDown( MouseEvent &event );
	//void mouseUp( MouseEvent &event );
	//void mouseWheel( MouseEvent &event );
	//void mouseDrag( MouseEvent &event );
	void mouseEvent( MouseEvent &event );

	void keyDown( KeyEvent &event );
	void keyUp( KeyEvent &event );

	void mouseDown( const vec2 &mousePos );
	void mouseUp( const vec2 &mousePos );
	void mouseWheel( float increment );
	//void mouseDrag( const vec2 &mousePos, bool leftDown, bool middleDown, bool rightDown );
	void mouseDrag( const vec2 &mousePos );

	//! stops movement
	void stop();
	//! updates movement smoothly
	void update();

private:
	bool      mEnabled = true;
	bool	  mMouseDown = false;
	vec3	  mMoveDirection, mMoveAccel, mMoveVelocity;
	float	  mMoveSpeed = 1.0f;

	vec2	  mWindowSize;
	vec2	  mInitialMousePos; // TODO: rename
	vec2	  mLookDelta;
	quat      mOrientationMouseDown;
};

} // namespace juniper