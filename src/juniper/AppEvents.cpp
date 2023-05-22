
#include "appEvents.h"
#include "Juniper.h"

#include "glfw/glfw3.h"
//#include "glad/glad.h"

#include <map>
using std::map;

namespace juniper {

namespace {

static bool sTableInited = false;
static const int MAX_KEYCODE = GLFW_KEY_LAST + 1;
KeyEvent::Key sKeyTable[MAX_KEYCODE];

} // anonymous namespace

static void initKeyTable()
{
	for( int c = 0; c < MAX_KEYCODE; ++c ) {
		sKeyTable[c] = KeyEvent::Key::KEY_UNKNOWN;
	}

	sKeyTable[GLFW_KEY_SPACE] = KeyEvent::Key::KEY_SPACE;
	sKeyTable[GLFW_KEY_APOSTROPHE] = KeyEvent::Key::KEY_QUOTE;  
	sKeyTable[GLFW_KEY_COMMA] = KeyEvent::Key::KEY_COMMA;  
	sKeyTable[GLFW_KEY_MINUS] = KeyEvent::Key::KEY_MINUS;  
	sKeyTable[GLFW_KEY_PERIOD] = KeyEvent::Key::KEY_PERIOD;  
	sKeyTable[GLFW_KEY_SLASH] = KeyEvent::Key::KEY_SLASH;  

	sKeyTable[GLFW_KEY_0] = KeyEvent::Key::KEY_0;
	sKeyTable[GLFW_KEY_1] = KeyEvent::Key::KEY_1;
	sKeyTable[GLFW_KEY_2] = KeyEvent::Key::KEY_2;
	sKeyTable[GLFW_KEY_3] = KeyEvent::Key::KEY_3;
	sKeyTable[GLFW_KEY_4] = KeyEvent::Key::KEY_4;
	sKeyTable[GLFW_KEY_5] = KeyEvent::Key::KEY_5;
	sKeyTable[GLFW_KEY_6] = KeyEvent::Key::KEY_6;
	sKeyTable[GLFW_KEY_7] = KeyEvent::Key::KEY_7;
	sKeyTable[GLFW_KEY_8] = KeyEvent::Key::KEY_8;
	sKeyTable[GLFW_KEY_9] = KeyEvent::Key::KEY_9;

	sKeyTable[GLFW_KEY_SEMICOLON] = KeyEvent::Key::KEY_SEMICOLON;  
	sKeyTable[GLFW_KEY_EQUAL] = KeyEvent::Key::KEY_EQUALS;  

	sKeyTable[GLFW_KEY_A] = KeyEvent::Key::KEY_a;
	sKeyTable[GLFW_KEY_B] = KeyEvent::Key::KEY_b;
	sKeyTable[GLFW_KEY_C] = KeyEvent::Key::KEY_c;
	sKeyTable[GLFW_KEY_D] = KeyEvent::Key::KEY_d;
	sKeyTable[GLFW_KEY_E] = KeyEvent::Key::KEY_e;
	sKeyTable[GLFW_KEY_F] = KeyEvent::Key::KEY_f;
	sKeyTable[GLFW_KEY_G] = KeyEvent::Key::KEY_g;
	sKeyTable[GLFW_KEY_H] = KeyEvent::Key::KEY_h;
	sKeyTable[GLFW_KEY_I] = KeyEvent::Key::KEY_i;
	sKeyTable[GLFW_KEY_J] = KeyEvent::Key::KEY_j;
	sKeyTable[GLFW_KEY_K] = KeyEvent::Key::KEY_k;
	sKeyTable[GLFW_KEY_L] = KeyEvent::Key::KEY_l;
	sKeyTable[GLFW_KEY_M] = KeyEvent::Key::KEY_m;
	sKeyTable[GLFW_KEY_N] = KeyEvent::Key::KEY_n;
	sKeyTable[GLFW_KEY_O] = KeyEvent::Key::KEY_o;
	sKeyTable[GLFW_KEY_P] = KeyEvent::Key::KEY_p;
	sKeyTable[GLFW_KEY_Q] = KeyEvent::Key::KEY_q;
	sKeyTable[GLFW_KEY_R] = KeyEvent::Key::KEY_r;
	sKeyTable[GLFW_KEY_S] = KeyEvent::Key::KEY_s;
	sKeyTable[GLFW_KEY_T] = KeyEvent::Key::KEY_t;
	sKeyTable[GLFW_KEY_U] = KeyEvent::Key::KEY_u;
	sKeyTable[GLFW_KEY_V] = KeyEvent::Key::KEY_v;
	sKeyTable[GLFW_KEY_W] = KeyEvent::Key::KEY_w;
	sKeyTable[GLFW_KEY_X] = KeyEvent::Key::KEY_x;
	sKeyTable[GLFW_KEY_Y] = KeyEvent::Key::KEY_y;
	sKeyTable[GLFW_KEY_Z] = KeyEvent::Key::KEY_z;

	sKeyTable[GLFW_KEY_BACKSLASH]     = KeyEvent::Key::KEY_BACKSLASH;  
	sKeyTable[GLFW_KEY_LEFT_BRACKET]  = KeyEvent::Key::KEY_LEFTBRACKET;  
	sKeyTable[GLFW_KEY_RIGHT_BRACKET] = KeyEvent::Key::KEY_RIGHTBRACKET;  
	sKeyTable[GLFW_KEY_GRAVE_ACCENT]  = KeyEvent::Key::KEY_BACKQUOTE;  
	// sKeyTable[GLFW_KEY_WORLD_1]       = KeyEvent::Key::KEY_WORLD_1; 
	// sKeyTable[GLFW_KEY_WORLD_2]       = KeyEvent::Key::KEY_WORLD_2; 
	sKeyTable[GLFW_KEY_ESCAPE]        = KeyEvent::Key::KEY_ESCAPE;
	sKeyTable[GLFW_KEY_ENTER]         = KeyEvent::Key::KEY_RETURN;
	sKeyTable[GLFW_KEY_TAB]           = KeyEvent::Key::KEY_TAB;
	sKeyTable[GLFW_KEY_BACKSPACE]     = KeyEvent::Key::KEY_BACKSPACE;
	sKeyTable[GLFW_KEY_INSERT]        = KeyEvent::Key::KEY_INSERT;
	sKeyTable[GLFW_KEY_DELETE]        = KeyEvent::Key::KEY_DELETE;
	sKeyTable[GLFW_KEY_RIGHT]         = KeyEvent::Key::KEY_RIGHT;
	sKeyTable[GLFW_KEY_LEFT]          = KeyEvent::Key::KEY_LEFT;
	sKeyTable[GLFW_KEY_DOWN]          = KeyEvent::Key::KEY_DOWN;
	sKeyTable[GLFW_KEY_UP]            = KeyEvent::Key::KEY_UP;
	sKeyTable[GLFW_KEY_PAGE_UP]       = KeyEvent::Key::KEY_PAGEUP;
	sKeyTable[GLFW_KEY_PAGE_DOWN]     = KeyEvent::Key::KEY_PAGEDOWN;
	sKeyTable[GLFW_KEY_HOME]          = KeyEvent::Key::KEY_HOME;
	sKeyTable[GLFW_KEY_END]           = KeyEvent::Key::KEY_END;
	sKeyTable[GLFW_KEY_CAPS_LOCK]     = KeyEvent::Key::KEY_CAPSLOCK;
	sKeyTable[GLFW_KEY_SCROLL_LOCK]   = KeyEvent::Key::KEY_SCROLLOCK;
	sKeyTable[GLFW_KEY_NUM_LOCK]      = KeyEvent::Key::KEY_NUMLOCK;
	sKeyTable[GLFW_KEY_PRINT_SCREEN]  = KeyEvent::Key::KEY_PRINT;
	sKeyTable[GLFW_KEY_PAUSE]         = KeyEvent::Key::KEY_PAUSE;

	sKeyTable[GLFW_KEY_F1] = KeyEvent::Key::KEY_F1;
	sKeyTable[GLFW_KEY_F2] = KeyEvent::Key::KEY_F2;
	sKeyTable[GLFW_KEY_F3] = KeyEvent::Key::KEY_F3;
	sKeyTable[GLFW_KEY_F4] = KeyEvent::Key::KEY_F4;
	sKeyTable[GLFW_KEY_F5] = KeyEvent::Key::KEY_F5;
	sKeyTable[GLFW_KEY_F6] = KeyEvent::Key::KEY_F6;
	sKeyTable[GLFW_KEY_F7] = KeyEvent::Key::KEY_F7;
	sKeyTable[GLFW_KEY_F8] = KeyEvent::Key::KEY_F8;
	sKeyTable[GLFW_KEY_F9] = KeyEvent::Key::KEY_F9;
	sKeyTable[GLFW_KEY_F10] = KeyEvent::Key::KEY_F10;
	sKeyTable[GLFW_KEY_F11] = KeyEvent::Key::KEY_F11;
	sKeyTable[GLFW_KEY_F12] = KeyEvent::Key::KEY_F12;
	sKeyTable[GLFW_KEY_F13] = KeyEvent::Key::KEY_F13;
	sKeyTable[GLFW_KEY_F14] = KeyEvent::Key::KEY_F14;
	sKeyTable[GLFW_KEY_F15] = KeyEvent::Key::KEY_F15;
	// sKeyTable[GLFW_KEY_F16] = KeyEvent::Key::KEY_F16;
	// sKeyTable[GLFW_KEY_F17] = KeyEvent::Key::KEY_F17;
	// sKeyTable[GLFW_KEY_F18] = KeyEvent::Key::KEY_F18;
	// sKeyTable[GLFW_KEY_F19] = KeyEvent::Key::KEY_F19;
	// sKeyTable[GLFW_KEY_F20] = KeyEvent::Key::KEY_F20;
	// sKeyTable[GLFW_KEY_F21] = KeyEvent::Key::KEY_F21;
	// sKeyTable[GLFW_KEY_F22] = KeyEvent::Key::KEY_F22;
	// sKeyTable[GLFW_KEY_F23] = KeyEvent::Key::KEY_F23;
	// sKeyTable[GLFW_KEY_F24] = KeyEvent::Key::KEY_F24;
	// sKeyTable[GLFW_KEY_F25] = KeyEvent::Key::KEY_F25;

	sKeyTable[GLFW_KEY_KP_0] = KeyEvent::Key::KEY_KP0;
	sKeyTable[GLFW_KEY_KP_1] = KeyEvent::Key::KEY_KP1;
	sKeyTable[GLFW_KEY_KP_2] = KeyEvent::Key::KEY_KP2;
	sKeyTable[GLFW_KEY_KP_3] = KeyEvent::Key::KEY_KP3;
	sKeyTable[GLFW_KEY_KP_4] = KeyEvent::Key::KEY_KP4;
	sKeyTable[GLFW_KEY_KP_5] = KeyEvent::Key::KEY_KP5;
	sKeyTable[GLFW_KEY_KP_6] = KeyEvent::Key::KEY_KP6;
	sKeyTable[GLFW_KEY_KP_7] = KeyEvent::Key::KEY_KP7;
	sKeyTable[GLFW_KEY_KP_8] = KeyEvent::Key::KEY_KP8;
	sKeyTable[GLFW_KEY_KP_9] = KeyEvent::Key::KEY_KP9;

	sKeyTable[GLFW_KEY_KP_DECIMAL]  = KeyEvent::Key::KEY_KP_PERIOD;
	sKeyTable[GLFW_KEY_KP_DIVIDE]   = KeyEvent::Key::KEY_KP_DIVIDE;
	sKeyTable[GLFW_KEY_KP_MULTIPLY] = KeyEvent::Key::KEY_KP_MULTIPLY;
	sKeyTable[GLFW_KEY_KP_SUBTRACT] = KeyEvent::Key::KEY_KP_MINUS;
	sKeyTable[GLFW_KEY_KP_ADD]      = KeyEvent::Key::KEY_KP_PLUS;
	sKeyTable[GLFW_KEY_KP_ENTER]    = KeyEvent::Key::KEY_KP_ENTER;
	sKeyTable[GLFW_KEY_KP_EQUAL]    = KeyEvent::Key::KEY_KP_EQUALS;

	sKeyTable[GLFW_KEY_LEFT_SHIFT]   = KeyEvent::Key::KEY_LSHIFT;
	sKeyTable[GLFW_KEY_LEFT_CONTROL] = KeyEvent::Key::KEY_LCTRL;
	sKeyTable[GLFW_KEY_LEFT_ALT]     = KeyEvent::Key::KEY_LALT;
	sKeyTable[GLFW_KEY_LEFT_SUPER]   = KeyEvent::Key::KEY_LSUPER;

	sKeyTable[GLFW_KEY_RIGHT_SHIFT]   = KeyEvent::Key::KEY_RSHIFT;
	sKeyTable[GLFW_KEY_RIGHT_CONTROL] = KeyEvent::Key::KEY_RCTRL;
	sKeyTable[GLFW_KEY_RIGHT_ALT]     = KeyEvent::Key::KEY_RALT;
	sKeyTable[GLFW_KEY_RIGHT_SUPER]   = KeyEvent::Key::KEY_RSUPER;

	sKeyTable[GLFW_KEY_MENU] = KeyEvent::Key::KEY_MENU;
}

KeyEvent::Key KeyEvent::translateNativeKeyCode( int nativeKeyCode )
{
	if( ! sTableInited )
		initKeyTable();

	if( nativeKeyCode < 0 || nativeKeyCode >= MAX_KEYCODE )
		return KeyEvent::Key::KEY_UNKNOWN;
	else
		return sKeyTable[nativeKeyCode];
}

// ----------------------------------------------------------------------------------
// Enum String Conversions
// ----------------------------------------------------------------------------------

const char* getAsString( const KeyEvent::State &s )
{
	switch( s ) {
		case KeyEvent::State::Release:		return "Release";
		case KeyEvent::State::Press:		return "Press";
		case KeyEvent::State::Repeat:		return "Repeat";
		default:							return "Unknown";
	}
}

const char* getAsString( const MouseEvent::State &s )
{
	switch( s ) {
		case MouseEvent::State::Release:	return "Release";
		case MouseEvent::State::Press:		return "Press";
		case MouseEvent::State::Move:		return "Move";
		case MouseEvent::State::Scroll:		return "Scroll";
		default:							return "Unknown";
	}
}

} // namespace juniper
