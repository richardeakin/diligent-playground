/*
 *  Copyright 2019-2022 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

#ifndef ENGINE_DLL
#    define ENGINE_DLL 1
#endif

#ifndef D3D11_SUPPORTED
#    define D3D11_SUPPORTED 0
#endif

#ifndef D3D12_SUPPORTED
#    define D3D12_SUPPORTED 0
#endif

#ifndef GL_SUPPORTED
#    define GL_SUPPORTED 0
#endif

#ifndef VULKAN_SUPPORTED
#    define VULKAN_SUPPORTED 0
#endif

#ifndef METAL_SUPPORTED
#    define METAL_SUPPORTED 0
#endif

#if PLATFORM_WIN32
#    define GLFW_EXPOSE_NATIVE_WIN32 1
#endif

#if PLATFORM_LINUX
#    define GLFW_EXPOSE_NATIVE_X11 1
#endif

#if PLATFORM_MACOS
#    define GLFW_EXPOSE_NATIVE_COCOA 1
#endif

#if D3D11_SUPPORTED
#    include "Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h"
#endif
#if D3D12_SUPPORTED
#    include "Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h"
#endif
#if GL_SUPPORTED
#    include "Graphics/GraphicsEngineOpenGL/interface/EngineFactoryOpenGL.h"
#endif
#if VULKAN_SUPPORTED
#    include "Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"
#endif
#if METAL_SUPPORTED
#    include "Graphics/GraphicsEngineMetal/interface/EngineFactoryMtl.h"
#endif

#ifdef GetObject
#    undef GetObject
#endif
#ifdef CreateWindow
#    undef CreateWindow
#endif

#include "AppGlfw.h"
#include "Juniper.h"
#include "ImGuiImplGlfw.h"

#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#ifdef GetObject
#    undef GetObject
#endif
#ifdef CreateWindow
#    undef CreateWindow
#endif


#if PLATFORM_MACOS
extern void* GetNSWindowView(GLFWwindow* wnd);
#endif

using namespace Diligent;

namespace juniper {

namespace {

//! Translates GLFW \a mods into ci::KeyEvent modifier values
static int extractKeyModifiers( int mods )
{
	uint32_t modifiers = 0;
	if( mods & GLFW_MOD_SHIFT ) {
		modifiers |= KeyEvent::SHIFT_DOWN;
	}
	if( mods & GLFW_MOD_CONTROL ) {
		modifiers |= KeyEvent::CTRL_DOWN;
	}
	if( mods & GLFW_MOD_ALT ) {
		modifiers |= KeyEvent::ALT_DOWN;
	}
	if( mods & GLFW_MOD_SUPER ) {
		modifiers |= KeyEvent::META_DOWN;
	}
	return modifiers;
}

//! Limit char8 to ASCII input for now.
static int8_t modifyCharToASCII( int key, uint32_t modifiers, bool capsLockIsDown )
{
	int8_t ret = ( key <= 127 ) ? (char)key : 0;
	if( ret ) {
		// a letter key
		bool shiftIsDown = modifiers & KeyEvent::SHIFT_DOWN;
		if( ret > 64 && ret < 91 && ! ( shiftIsDown ^ capsLockIsDown ) ) {
			ret += 32;
		}
		// other modifiable keys
		else if( shiftIsDown ) {
			switch ( ret ) {
			case '1': ret = '!'; break;
			case '2': ret = '@'; break;
			case '3': ret = '#'; break;
			case '4': ret = '$'; break;
			case '5': ret = '%'; break;
			case '6': ret = '^'; break;
			case '7': ret = '&'; break;
			case '8': ret = '*'; break;
			case '9': ret = '('; break;
			case '0': ret = ')'; break;
			case '`': ret = '~'; break;
			case '-': ret = '_'; break;
			case '=': ret = '+'; break;
			case '[': ret = '{'; break;
			case ']': ret = '}'; break;
			case '\\': ret = '|'; break;
			case ';': ret = ':'; break;
			case '\'': ret = '"'; break;
			case ',': ret = '<'; break;
			case '.': ret = '>'; break;
			case '/': ret = '?'; break;
			default: break;
			};
		}
	}
	return ret;
}

} // anonymous namespace

// ----------------------------------------------------------------------------------
// Init / Destroy
// ----------------------------------------------------------------------------------

AppGlfw::AppGlfw()
{
}

AppGlfw::~AppGlfw()
{
	if( mImmediateContext ) {
		mImmediateContext->Flush();
	}

	mSwapChain = nullptr;
	mImmediateContext = nullptr;
	mRenderDevice = nullptr;

	if( mWindow ) {
		glfwDestroyWindow( mWindow );
		glfwTerminate();
	}
}

// pick a default RENDER_DEVICE_TYPE (user can override
RENDER_DEVICE_TYPE AppGlfw::chooseDefaultRenderDeviceType() const
{
#if METAL_SUPPORTED
	return RENDER_DEVICE_TYPE_METAL;
#elif D3D12_SUPPORTED
	return RENDER_DEVICE_TYPE_D3D12;
#elif VULKAN_SUPPORTED
	return RENDER_DEVICE_TYPE_VULKAN;
#elif D3D11_SUPPORTED
	return RENDER_DEVICE_TYPE_D3D11;
#elif GL_SUPPORTED
	return RENDER_DEVICE_TYPE_GL;
#else
	return RENDER_DEVICE_TYPE_UNDEFINED;
#endif
}

bool AppGlfw::createWindow( const AppSettings &settings, int glfwApiHint )
{
    glfwSetErrorCallback( GLFW_errorCallback );

	if( glfwInit() != GLFW_TRUE )
		return false;

	glfwWindowHint( GLFW_CLIENT_API, glfwApiHint );
	if( glfwApiHint == GLFW_OPENGL_API ) {
		// We need compute shaders, so request OpenGL 4.2 at least
		glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
		glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 2 );
	}

	mWindow = glfwCreateWindow( settings.windowSize.x, settings.windowSize.y, settings.title.c_str(), nullptr, nullptr );
	if( mWindow == nullptr ) {
		LOG_ERROR_MESSAGE( "Failed to create GLFW window" );
		return false;
	}

	if( settings.monitorIndex > 0 ) {
		int           monitorCount = 0;
		GLFWmonitor** monitors = glfwGetMonitors( &monitorCount );

		if( monitorCount > settings.monitorIndex ) {
			// move our window to the right of the primary monitor
			int xpos, ypos, width, height;
			glfwGetMonitorPos( monitors[settings.monitorIndex], &xpos, &ypos );
			glfwSetWindowPos( mWindow, xpos + settings.windowPos.x, ypos + settings.windowPos.y );

			// TODO: use fullScreen settings flag
			// TODO: this will have to be improved but good enough for now
			// - window chrome is sitting above
			//if( FULLSCREEN_WINDOW ) {
			//	glfwGetMonitorWorkarea( monitors[1], &xpos, &ypos, &width, &height );
			//	glfwSetWindowSize( m_Window, width, height );
			//}
		}
		else {
			JU_LOG_WARNING( "settings.monitorIndex out of range (", settings.monitorIndex, ")" );
		}
	}
	else {
		glfwSetWindowPos( mWindow, settings.windowPos.x, settings.windowPos.y );
	}

	// TODO: add glfwSetCharCallback, glfwSetDropCallback

	glfwSetWindowUserPointer( mWindow, this );
	glfwSetFramebufferSizeCallback( mWindow, &GLFW_ResizeCallback );
	glfwSetKeyCallback( mWindow, &GLFW_KeyCallback );
	glfwSetCharCallback( mWindow, &GLFW_CharCallback );
	glfwSetMouseButtonCallback( mWindow, &GLFW_MouseButtonCallback );
	glfwSetCursorPosCallback( mWindow, &GLFW_CursorPosCallback );
	glfwSetScrollCallback( mWindow, &GLFW_MouseWheelCallback );

	glfwSetWindowSizeLimits( mWindow, 320, 240, GLFW_DONT_CARE, GLFW_DONT_CARE );
	return true;
}

bool AppGlfw::initEngine( RENDER_DEVICE_TYPE DevType )
{
#if PLATFORM_WIN32
	Win32NativeWindow Window{ glfwGetWin32Window( mWindow ) };
#endif
#if PLATFORM_LINUX
	LinuxNativeWindow Window;
	Window.WindowId = glfwGetX11Window( m_Window );
	Window.pDisplay = glfwGetX11Display();
	if( DevType == RENDER_DEVICE_TYPE_GL )
		glfwMakeContextCurrent( m_Window );
#endif
#if PLATFORM_MACOS
	MacOSNativeWindow Window;
	if( DevType == RENDER_DEVICE_TYPE_GL )
		glfwMakeContextCurrent( m_Window );
	else
		Window.pNSView = GetNSWindowView( m_Window );
#endif

	SwapChainDesc SCDesc;
	switch( DevType ) {
#if D3D11_SUPPORTED
	case RENDER_DEVICE_TYPE_D3D11: {
#    if ENGINE_DLL
		// Load the dll and import GetEngineFactoryD3D11() function
		auto* GetEngineFactoryD3D11 = LoadGraphicsEngineD3D11();
#    endif
		auto* pFactoryD3D11 = GetEngineFactoryD3D11();

		EngineD3D11CreateInfo EngineCI;
		pFactoryD3D11->CreateDeviceAndContextsD3D11( EngineCI, &mRenderDevice, &mImmediateContext );
		pFactoryD3D11->CreateSwapChainD3D11( mRenderDevice, mImmediateContext, SCDesc, FullScreenModeDesc{}, Window, &mSwapChain );
	}
	break;
#endif // D3D11_SUPPORTED


#if D3D12_SUPPORTED
	case RENDER_DEVICE_TYPE_D3D12: {
#    if ENGINE_DLL
		// Load the dll and import GetEngineFactoryD3D12() function
		auto* GetEngineFactoryD3D12 = LoadGraphicsEngineD3D12();
#    endif
		auto* pFactoryD3D12 = GetEngineFactoryD3D12();

		EngineD3D12CreateInfo EngineCI;
		pFactoryD3D12->CreateDeviceAndContextsD3D12( EngineCI, &mRenderDevice, &mImmediateContext );
		pFactoryD3D12->CreateSwapChainD3D12( mRenderDevice, mImmediateContext, SCDesc, FullScreenModeDesc{}, Window, &mSwapChain );
	}
	break;
#endif // D3D12_SUPPORTED


#if GL_SUPPORTED
	case RENDER_DEVICE_TYPE_GL:
	{
#    if EXPLICITLY_LOAD_ENGINE_GL_DLL
		// Load the dll and import GetEngineFactoryOpenGL() function
		auto GetEngineFactoryOpenGL = LoadGraphicsEngineOpenGL();
#    endif
		auto* pFactoryOpenGL = GetEngineFactoryOpenGL();

		EngineGLCreateInfo EngineCI;
		EngineCI.Window = Window;
		pFactoryOpenGL->CreateDeviceAndSwapChainGL( EngineCI, &mRenderDevice, &mImmediateContext, SCDesc, &mSwapChain );
	}
	break;
#endif // GL_SUPPORTED


#if VULKAN_SUPPORTED
	case RENDER_DEVICE_TYPE_VULKAN:
	{
#    if EXPLICITLY_LOAD_ENGINE_VK_DLL
		// Load the dll and import GetEngineFactoryVk() function
		auto* GetEngineFactoryVk = LoadGraphicsEngineVk();
#    endif
		auto* pFactoryVk = GetEngineFactoryVk();

		EngineVkCreateInfo EngineCI;
		pFactoryVk->CreateDeviceAndContextsVk( EngineCI, &mRenderDevice, &mImmediateContext );
		pFactoryVk->CreateSwapChainVk( mRenderDevice, mImmediateContext, SCDesc, Window, &mSwapChain );
	}
	break;
#endif // VULKAN_SUPPORTED

#if METAL_SUPPORTED
	case RENDER_DEVICE_TYPE_METAL:
	{
		auto* pFactoryMtl = GetEngineFactoryMtl();

		EngineMtlCreateInfo EngineCI;
		pFactoryMtl->CreateDeviceAndContextsMtl( EngineCI, &m_pDevice, &m_pImmediateContext );
		pFactoryMtl->CreateSwapChainMtl( m_pDevice, m_pImmediateContext, SCDesc, Window, &m_pSwapChain );
	}
	break;
#endif // METAL_SUPPORTED

	default:
		std::cerr << "Unknown/unsupported device type";
		return false;
		break;
	}

	if( mRenderDevice == nullptr || mImmediateContext == nullptr || mSwapChain == nullptr )
		return false;

	// TODO: call virtual resize() here, or make sure it gets called once after initialize()

	return true;
}

void AppGlfw::initImGui()
{
	auto scDesc = getSwapChain()->GetDesc();
	mImGui.reset( new ImGuiImplGlfw( mWindow, getDevice(), scDesc.ColorBufferFormat, scDesc.DepthBufferFormat ) );
}

void AppGlfw::quit()
{
	VERIFY_EXPR( mWindow != nullptr );
	glfwSetWindowShouldClose( mWindow, GLFW_TRUE );
}

// ----------------------------------------------------------------------------------
// GLFW Callbacks
// ----------------------------------------------------------------------------------

void AppGlfw::GLFW_ResizeCallback( GLFWwindow* wnd, int w, int h )
{
	auto* self = static_cast<AppGlfw*>( glfwGetWindowUserPointer( wnd ) );
	if( self->mSwapChain != nullptr ) {
		self->mSwapChain->Resize( static_cast<Uint32>( w ), static_cast<Uint32>( h ) );
		self->resize( { w, h } );
	}
}

void AppGlfw::GLFW_KeyCallback( GLFWwindow* window, int key, int scancode, int action, int mods )
{
	auto* self = static_cast<AppGlfw*>( glfwGetWindowUserPointer( window ) );

	auto modifiers = extractKeyModifiers( mods );
	bool capsLock = glfwGetKey( window, GLFW_KEY_CAPS_LOCK );
	auto convertedChar = modifyCharToASCII( key, modifiers, capsLock );
	auto state = KeyEvent::State::Unknown;
	auto keyTranslated = KeyEvent::translateNativeKeyCode( key );
	bool processCallback = true;

	KeyEvent keyEvent;
	if( action == GLFW_PRESS ) {
		state = KeyEvent::State::Press;
		// if convertedChar != 0, we'll get the unicode char value and wait until onCharInput is called.
		// if convertedChar == 0, that means it is a non-unicode key (ex ctrl), so we'll issue a keydown here.
		if( convertedChar != 0 ) {
			processCallback = false;
		}

		//JU_LOG_INFO( "press key: ", (int)keyTranslated, ", scancode: ", scancode );

		keyEvent = KeyEvent( keyTranslated, 0, convertedChar, state, modifiers, scancode );
	}
	else if( action == GLFW_RELEASE ) {
		KeyEvent* active = self->findActiveKeyEvent( scancode );
		VERIFY_EXPR( active != nullptr );

		state = KeyEvent::State::Release;
		keyEvent = KeyEvent( keyTranslated, active->getCharUtf32(), convertedChar, state, modifiers, scancode );

		//JU_LOG_INFO( "release key: ", (int)keyTranslated, ", scancode: ", scancode, ", charUtf32: ", keyEvent.getCharUtf32() );
	}
	else if( action == GLFW_REPEAT ) {
		KeyEvent* active = self->findActiveKeyEvent( scancode );
		VERIFY_EXPR( active != nullptr );

		state = KeyEvent::State::Repeat;
		keyEvent = KeyEvent( keyTranslated, active->getCharUtf32(), convertedChar, state, modifiers, scancode );

		JU_LOG_INFO( "repeat key: ", (int)keyTranslated, ", scancode: ", scancode, ", charUtf32: ", keyEvent.getCharUtf32() );
	}

	self->addOrUpdateKeyEvent( keyEvent );

	if( processCallback ) {
		self->keyEvent( keyEvent );
	}
}

void AppGlfw::GLFW_CharCallback( GLFWwindow *window, unsigned int codepoint )
{
	auto* self = static_cast<AppGlfw*>( glfwGetWindowUserPointer( window ) );

	// find active KeyEvent and update with utf32 char, issue event with that.
	char asciiChar = codepoint < 256 ? (char)codepoint : 0;

	//JU_LOG_INFO( "last key: ", (int)self->mLastKeyEvent.getKey(), ", asciiChar: ", asciiChar, ", codepoint: ", codepoint );

	auto keyEvent = KeyEvent( self->mLastKeyEvent.getKey(), codepoint, asciiChar, self->mLastKeyEvent.getState(), self->mLastKeyEvent.getModifiers(), self->mLastKeyEvent.getNativeKeyCode() );
	self->addOrUpdateKeyEvent( keyEvent );
	self->keyEvent( keyEvent );
}

void AppGlfw::GLFW_MouseButtonCallback( GLFWwindow* wnd, int button, int state, int )
{
	auto* self = static_cast<AppGlfw*>( glfwGetWindowUserPointer( wnd ) );

	// TODO: handle as a mouseEvent
	//self->onKeyEvent(static_cast<Key>(button), static_cast<KeyState>(state));

}

void AppGlfw::GLFW_CursorPosCallback( GLFWwindow* wnd, double xpos, double ypos )
{
	float xscale = 1;
	float yscale = 1;
	glfwGetWindowContentScale( wnd, &xscale, &yscale );
	auto* self = static_cast<AppGlfw*>( glfwGetWindowUserPointer( wnd ) );
	self->mouseEvent( float2( static_cast<float>( xpos * xscale ), static_cast<float>( ypos * yscale ) ) );
}

void AppGlfw::GLFW_MouseWheelCallback(GLFWwindow* wnd, double dx, double dy)
{
    JU_LOG_INFO( "dx: ", dx, ", dy: ", dy);
	// TODO: call to app virtual functino
}

void AppGlfw::GLFW_errorCallback( int error, const char *description )
{
    JU_LOG_ERROR( "GLFW error: ", error, ", description: ", description );
}

// ----------------------------------------------------------------------------------
// Event Handling
// ----------------------------------------------------------------------------------

KeyEvent* AppGlfw::findActiveKeyEvent( int nativeCode )
{
	for( auto &active : mActiveKeys ) {
		if( active.getNativeKeyCode() == nativeCode ) {
			return &active;
		}
	}

	return nullptr;
}

void AppGlfw::addOrUpdateKeyEvent( const KeyEvent &key )
{
	mLastKeyEvent = key;

	// update if active
	// TODO: use iter
	for( int i = 0; i < mActiveKeys.size(); i++ ) {
		const auto &active = mActiveKeys[i];
		if( active.getKey() == key.getKey() ) {
			mActiveKeys[i] = key;
			return;
		}
	}

	mActiveKeys.push_back( key );
}

void AppGlfw::flushOldKeyEvents()
{
	for( auto keyIter = mActiveKeys.begin(); keyIter != mActiveKeys.end(); ) {
		if( keyIter->getState() == KeyEvent::State::Release ) {
			keyIter = mActiveKeys.erase( keyIter );
		}
		else {
			++keyIter;                             
		}
	}
}

// ----------------------------------------------------------------------------------
// Main Loop
// ----------------------------------------------------------------------------------

void AppGlfw::loop()
{
    mLastUpdate = TClock::now();
    for( ; ; ) {
		if( glfwWindowShouldClose( mWindow ) ) {
			return;
		}

		flushOldKeyEvents();

        glfwPollEvents();


        const auto time = TClock::now();
        const auto dt   = std::chrono::duration_cast<TSeconds>( time - mLastUpdate ).count();
        mLastUpdate    = time;

        updateEntry( dt );

        int w, h;
        glfwGetWindowSize( mWindow , &w, &h );

        // Skip rendering if window is minimized or too small
        if( w > 0 && h > 0 ) {
            drawEntry();
		}
    }
}

// ----------------------------------------------------------------------------------
// Main
// ----------------------------------------------------------------------------------

int AppGlfwMain( int argc, const char* const* argv )
{
	std::unique_ptr<AppGlfw> app{ CreateGLFWApp() };

	AppSettings settings;
	settings.renderDeviceType = app->chooseDefaultRenderDeviceType();
	app->prepareSettings( &settings );

	if( settings.title.empty() ) {
		// set a default title
		std::string title = app->getTitle();
		switch( settings.renderDeviceType ) {
			case RENDER_DEVICE_TYPE_D3D11:
				title.append( " (D3D11" );
			break;
			case RENDER_DEVICE_TYPE_D3D12:
				title.append( " (D3D12" );
			break;
			case RENDER_DEVICE_TYPE_GL:
				title.append( " (GL" );
			break;
			case RENDER_DEVICE_TYPE_VULKAN:
				title.append( " (VK" );
			break;
			case RENDER_DEVICE_TYPE_METAL:
				title.append( " (Metal" );
			break;
			default:
				UNEXPECTED( "Unexpected device type" );
		}
		title.append( ", API " );
		title.append( std::to_string( DILIGENT_API_VERSION ) );
		title.push_back( ')' );

		settings.title = title;
	}

	int APIHint = GLFW_NO_API;
#if !PLATFORM_WIN32
	if( DevType == RENDER_DEVICE_TYPE_GL ) {
		// On platforms other than Windows Diligent Engine
		// attaches to existing OpenGL context
		APIHint = GLFW_OPENGL_API;
	}
#endif

	if( ! app->createWindow( settings, APIHint ) )
		return -1;

	if( ! app->initEngine( settings.renderDeviceType ) )
		return -1;

	app->initImGui();
	app->initialize();
	app->loop();

	return 0;
}

} // namespace juniper

#if PLATFORM_WIN32
//int __stdcall WinMain( HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/ )
int WINAPI WinMain( HINSTANCE /*instance*/, HINSTANCE, LPSTR, int /*cmdShow*/ )
{
#if defined(_DEBUG) || defined(DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	// TODO: use this
	//const auto* cmdLine = GetCommandLineA();

	int argc = 0;
	const char* argv = nullptr;
	return juniper::AppGlfwMain( argc, &argv );
}
#else
int main( int argc, const char** argv )
{
    return juniper::AppGlfwMain( argc, argv );
}
#endif
