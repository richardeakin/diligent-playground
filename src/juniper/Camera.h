#pragma once

#include "cinder/Matrix.h"
#include "juniper/AppEvents.h"

namespace juniper {

using glm::vec2;
using glm::vec3;
using glm::mat4;

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
	void	setFov( float verticalFov ) { mFov = verticalFov;  mProjectionCached = false; }

	const mat4& getViewMatrix() const		{ return mViewMatrix; }
	const mat4& getProjectionMatrix() const	{ return mProjectionMatrix; }

	/*
	float3 GetWorldRight() const { return float3(m_ViewMatrix._11, m_ViewMatrix._21, m_ViewMatrix._31); }
	float3 GetWorldUp()    const { return float3(m_ViewMatrix._12, m_ViewMatrix._22, m_ViewMatrix._32); }
	float3 GetWorldAhead() const { return float3(m_ViewMatrix._13, m_ViewMatrix._23, m_ViewMatrix._33); }
	*/

	// TODO NEXT: test this in gui
	vec3 getWorldRight() const		{ return { mViewMatrix[1][1], mViewMatrix[1][2], mViewMatrix[1][3] }; }
	vec3 getWorldUp() const			{ return { mViewMatrix[2][1], mViewMatrix[2][2], mViewMatrix[3][2] }; }
	vec3 getWorldForward() const	{ return { mViewMatrix[3][1], mViewMatrix[3][2], mViewMatrix[3][3] }; }

protected:
	vec3 mEyeOrigin = { 0, 0, -5 };
	vec3 mEyeTarget = { 0, 0, 0 };
	vec3 mWorldUp = { 0, 1, 0 };

	float mFov = 40; // sample code is using a value of fovy = 0.7854
	float mAspectRatio = 1.0f;
	float mNearClip = 0.1f;
	float mFarClip = 100.0f;

	// TODO: use bools to minimize number of calcs
	bool mProjectionCached = false;
	bool mViewCached = false;
	mat4 mViewMatrix;
	mat4 mProjectionMatrix;
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

	//! updates movement smoothly
	void update();

private:
	bool      mEnabled = true;
	bool	  mLookEnabled = false; // TODO: rename to make more sense
	vec3	  mMoveDirection, mMoveAccel, mMoveVelocity;
	float	  mMoveSpeed = 1.0f;

	vec2	  mWindowSize;
	vec2	  mInitialMousePos, mLookDelta;
};

} // namespace juniper