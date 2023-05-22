
#pragma once

#include <cstdint>
#include "BasicMath.hpp"

namespace juniper {

namespace dg = Diligent;

//! Represents a keyboard event
class KeyEvent {
public:
	// TODO: update this to use enum class
	enum {	SHIFT_DOWN	= 0x0008,
		ALT_DOWN	= 0x0010,
		CTRL_DOWN	= 0x0020,
		META_DOWN	= 0x0040,
#if defined( CINDER_MSW )
		ACCEL_DOWN	= CTRL_DOWN
#else
		ACCEL_DOWN	= META_DOWN
#endif
	};

	enum class State {
		Release = 0,
		Press,
		Repeat,
		Unknown
	};

	// Key codes
	// TODO: remove unnecessary KEY_ prefixes (will have to leave those for numbers
	enum class Key {
		KEY_UNKNOWN		= 0,
		KEY_FIRST		= 0,
		KEY_BACKSPACE	= 8,
		KEY_TAB			= 9,
		KEY_CLEAR		= 12,
		KEY_RETURN		= 13,
		KEY_PAUSE		= 19,
		KEY_ESCAPE		= 27,
		KEY_SPACE		= 32,
		KEY_EXCLAIM		= 33,
		KEY_QUOTEDBL	= 34,
		KEY_HASH		= 35,
		KEY_DOLLAR		= 36,
		KEY_AMPERSAND	= 38,
		KEY_QUOTE		= 39,
		KEY_LEFTPAREN	= 40,
		KEY_RIGHTPAREN	= 41,
		KEY_ASTERISK	= 42,
		KEY_PLUS		= 43,
		KEY_COMMA		= 44,
		KEY_MINUS		= 45,
		KEY_PERIOD		= 46,
		KEY_SLASH		= 47,
		KEY_0			= 48,
		KEY_1			= 49,
		KEY_2			= 50,
		KEY_3			= 51,
		KEY_4			= 52,
		KEY_5			= 53,
		KEY_6			= 54,
		KEY_7			= 55,
		KEY_8			= 56,
		KEY_9			= 57,
		KEY_COLON		= 58,
		KEY_SEMICOLON	= 59,
		KEY_LESS		= 60,
		KEY_EQUALS		= 61,
		KEY_GREATER		= 62,
		KEY_QUESTION	= 63,
		KEY_AT			= 64,

		KEY_LEFTBRACKET	= 91,
		KEY_BACKSLASH	= 92,
		KEY_RIGHTBRACKET= 93,
		KEY_CARET		= 94,
		KEY_UNDERSCORE	= 95,
		KEY_BACKQUOTE	= 96,
		KEY_a			= 97,
		KEY_b			= 98,
		KEY_c			= 99,
		KEY_d			= 100,
		KEY_e			= 101,
		KEY_f			= 102,
		KEY_g			= 103,
		KEY_h			= 104,
		KEY_i			= 105,
		KEY_j			= 106,
		KEY_k			= 107,
		KEY_l			= 108,
		KEY_m			= 109,
		KEY_n			= 110,
		KEY_o			= 111,
		KEY_p			= 112,
		KEY_q			= 113,
		KEY_r			= 114,
		KEY_s			= 115,
		KEY_t			= 116,
		KEY_u			= 117,
		KEY_v			= 118,
		KEY_w			= 119,
		KEY_x			= 120,
		KEY_y			= 121,
		KEY_z			= 122,
		KEY_DELETE		= 127,

		KEY_KP0			= 256,
		KEY_KP1			= 257,
		KEY_KP2			= 258,
		KEY_KP3			= 259,
		KEY_KP4			= 260,
		KEY_KP5			= 261,
		KEY_KP6			= 262,
		KEY_KP7			= 263,
		KEY_KP8			= 264,
		KEY_KP9			= 265,
		KEY_KP_PERIOD	= 266,
		KEY_KP_DIVIDE	= 267,
		KEY_KP_MULTIPLY	= 268,
		KEY_KP_MINUS	= 269,
		KEY_KP_PLUS		= 270,
		KEY_KP_ENTER	= 271,
		KEY_KP_EQUALS	= 272,

		KEY_UP			= 273,
		KEY_DOWN		= 274,
		KEY_RIGHT		= 275,
		KEY_LEFT		= 276,
		KEY_INSERT		= 277,
		KEY_HOME		= 278,
		KEY_END			= 279,
		KEY_PAGEUP		= 280,
		KEY_PAGEDOWN	= 281,

		KEY_F1			= 282,
		KEY_F2			= 283,
		KEY_F3			= 284,
		KEY_F4			= 285,
		KEY_F5			= 286,
		KEY_F6			= 287,
		KEY_F7			= 288,
		KEY_F8			= 289,
		KEY_F9			= 290,
		KEY_F10			= 291,
		KEY_F11			= 292,
		KEY_F12			= 293,
		KEY_F13			= 294,
		KEY_F14			= 295,
		KEY_F15			= 296,

		KEY_NUMLOCK		= 300,
		KEY_CAPSLOCK	= 301,
		KEY_SCROLLOCK	= 302,
		KEY_RSHIFT		= 303,
		KEY_LSHIFT		= 304,
		KEY_RCTRL		= 305,
		KEY_LCTRL		= 306,
		KEY_RALT		= 307,
		KEY_LALT		= 308,
		KEY_RMETA		= 309,
		KEY_LMETA		= 310,
		KEY_LSUPER		= 311,		/* Left "Windows" key */
		KEY_RSUPER		= 312,		/* Right "Windows" key */
		KEY_MODE		= 313,		/* "Alt Gr" key */
		KEY_COMPOSE		= 314,		/* Multi-key compose key */

		KEY_HELP		= 315,
		KEY_PRINT		= 316,
		KEY_SYSREQ		= 317,
		KEY_BREAK		= 318,
		KEY_MENU		= 319,
		KEY_POWER		= 320,		/* Power Macintosh power key */
		KEY_EURO		= 321,		/* Some european keyboards */
		KEY_UNDO		= 322,		/* Atari keyboard has Undo */

		KEY_LAST
	};

	KeyEvent() = default;
	KeyEvent( Key key, uint32_t c32, char c, State state, unsigned int modifiers, unsigned int nativeKeyCode )
		: mKey( key ), mChar32( c32 ), mChar( c ), mState( state ), mModifiers( modifiers ), mNativeKeyCode( nativeKeyCode )
	{}

	//! Returns the ASCII character associated with the event.
	char			getChar() const					{ return mChar; }
	//! Returns the UTF-32 character associated with the event.
	uint32_t		getCharUtf32() const			{ return mChar32; } 
	//! Returns the key code associated with the event, which maps into the enum listed below
	Key				getKey() const					{ return mKey; }
	//! Returns the modifiers associated with the event. Can also use the convenience methods below to check for a specific modifiered,
	unsigned int	getModifiers() const			{ return mModifiers; }
	//! Returns whether the Shift key was pressed during the event.
	bool			isShiftDown() const				{ return (mModifiers & SHIFT_DOWN) ? true : false; }
	//! Returns whether the Alt (or Option) key was pressed during the event.
	bool			isAltDown() const				{ return (mModifiers & ALT_DOWN) ? true : false; }
	//! Returns whether the Control key was pressed during the event.
	bool			isControlDown() const			{ return (mModifiers & CTRL_DOWN) ? true : false; }
	//! Returns whether the meta key was pressed during the event. Maps to the Windows key on Windows and the Command key on Mac OS X.
	bool			isMetaDown() const				{ return (mModifiers & META_DOWN) ? true : false; }	
	//! Returns whether the accelerator key was pressed during the event. Maps to the Control key on Windows and the Command key on Mac OS X.
	bool			isAccelDown() const				{ return (mModifiers & ACCEL_DOWN) ? true : false; }	
	//! Returns the platform-native key-code.
	int				getNativeKeyCode() const		{ return mNativeKeyCode; }
	//! Returns the state of this keyboard event
	State			getState() const				{ return mState; }
	//! Update the state of this keyboard event
	//void			setState( const State &state )	{ mState = state; }

	// TODO: should probably live in AppGlfw if I want to keep this class generic
	static Key		translateNativeKeyCode( int nativeKeyCode );

protected:
	Key				mKey = Key::KEY_LAST;
	uint32_t		mChar32 = 0;
	char			mChar = 0; // TODO: get rid and convert internally if needed
	State			mState = State::Unknown;
	unsigned int	mModifiers = 0;
	unsigned int	mNativeKeyCode = 0;
};

// TODO: use this (right now just sketching it out
class MouseEvent {
public:
	enum class State {
		Press,
		Release,
		Move,
		Wheel, // TODO: send from callback
		Unknown
	};

	MouseEvent() = default;
	MouseEvent( const dg::float2 &pos, State state, int buttonIndex )
		: mPos( pos ), mState( state ), mButtonIndex( buttonIndex )
	{}

	//! Returns the state of this event
	State				getState() const		{ return mState; }
	//! Returns the cursor position of this event
	const dg::float2&	getPos() const			{ return mPos; }
	//! Returnns the index of the last pressed button, or -1 if none are down
	int					getButtonIndex() const	{ return mButtonIndex; }
	//! Returns if this event can be considered a mouse drag
	bool				isDrag() const			{ return mState == State::Move && mButtonIndex == -1; }

	// TODO: may use this
	//! Returns whether the initiator for the event was the left mouse button
//	bool		isLeft() const				{ return ( mInitiator & LEFT_DOWN ) ? true : false; }
//	//! Returns whether the initiator for the event was the right mouse button
//	bool		isRight() const				{ return ( mInitiator & RIGHT_DOWN ) ? true : false; }
//	//! Returns whether the initiator for the event was the middle mouse button
//	bool		isMiddle() const			{ return ( mInitiator & MIDDLE_DOWN ) ? true : false; }
//	//! Returns whether the left mouse button was pressed during the event
//	bool		isLeftDown() const			{ return (mModifiers & LEFT_DOWN) ? true : false; }
//	//! Returns whether the right mouse button was pressed during the event
//	bool		isRightDown() const			{ return (mModifiers & RIGHT_DOWN) ? true : false; }
//	//! Returns whether the middle mouse button was pressed during the event
//	bool		isMiddleDown() const		{ return (mModifiers & MIDDLE_DOWN) ? true : false; }
//	//! Returns whether the Shift key was pressed during the event.
//	bool		isShiftDown() const			{ return (mModifiers & SHIFT_DOWN) ? true : false; }
//	//! Returns whether the Alt (or Option) key was pressed during the event.
//	bool		isAltDown() const			{ return (mModifiers & ALT_DOWN) ? true : false; }
//	//! Returns whether the Control key was pressed during the event.
//	bool		isControlDown() const		{ return (mModifiers & CTRL_DOWN) ? true : false; }
//	//! Returns whether the meta key was pressed during the event. Maps to the Windows key on Windows and the Command key on Mac OS X.
//	bool		isMetaDown() const			{ return (mModifiers & META_DOWN) ? true : false; }
//	//! Returns whether the accelerator key was pressed during the event. Maps to the Control key on Windows and the Command key on Mac OS X.
//	bool		isAccelDown() const			{ return (mModifiers & ACCEL_DOWN) ? true : false; }
//	//! Returns the number of detents the user has wheeled through. Positive values correspond to wheel-up and negative to wheel-down.
//	float		getWheelIncrement() const	{ return mWheelIncrement; }
//
//	//! Returns the platform-native modifier mask
//	uint32_t	getNativeModifiers() const	{ return mNativeModifiers; }
//
//	enum {
//		LEFT_DOWN	= 0x0001,
//		RIGHT_DOWN	= 0x0002,
//		MIDDLE_DOWN = 0x0004,
//		SHIFT_DOWN	= 0x0008,
//		ALT_DOWN	= 0x0010,
//		CTRL_DOWN	= 0x0020,
//		META_DOWN	= 0x0040,
//#if defined( CINDER_MSW )
//		ACCEL_DOWN	= CTRL_DOWN
//#else
//		ACCEL_DOWN	= META_DOWN
//#endif
//	};

protected:
	dg::float2	mPos			= { 0, 0 };
	State		mState			= State::Unknown;
	int			mButtonIndex	= -1;
};


// TODO: add for KeyEvent (will add a key string to the internal map)
// TODO: add ostream<< versions

const char* getAsString( const MouseEvent::State &s );

} // namespace juniper
