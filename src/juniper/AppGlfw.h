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

#pragma once

#include "AppEvents.h"

#include "RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "BasicMath.hpp"

#include <chrono>
#include <vector>

typedef struct GLFWwindow GLFWwindow;

namespace Diligent {
    class ImGuiImplDiligent;
}

namespace juniper {

namespace dg = Diligent;
using dg::int2;
using dg::int3;
using dg::int4;
using dg::float2;
using dg::float3;
using dg::float4;
using dg::float4x4;
using dg::RefCntAutoPtr;

// TODO: make inner structA of AppGlfw, AppBasic will inherit from that for common things not needed by glfw
struct AppSettings {
    int2 windowPos                      = { 0, 0 }; // TODO: set so it is down a bit and you can see the title bar
    int2 windowSize                     = { 1024, 768 };
    int  monitorIndex                    = 0;
    dg::RENDER_DEVICE_TYPE renderDeviceType   = dg::RENDER_DEVICE_TYPE_UNDEFINED;
    std::string title;
};

class AppGlfw {
public:
    AppGlfw();
    virtual ~AppGlfw();

    // Public API
    const dg::IEngineFactory* getEngineFactory() const  { return mRenderDevice->GetEngineFactory(); }
    dg::IRenderDevice*        getDevice()               { return mRenderDevice; }
    const dg::IRenderDevice*  getDevice() const         { return mRenderDevice; }
    dg::IDeviceContext*       getContext()              { return mImmediateContext; }
    const dg::IDeviceContext* getContext() const        { return mImmediateContext; }
    dg::ISwapChain*           getSwapChain()            { return mSwapChain; }
    const dg::ISwapChain*     getSwapChain() const      { return mSwapChain; }

    void quit();

    // Required virtual:
     
    //! setups up AppGlobal and calls App::initialize()
    virtual void initEntry() = 0;
    //! Entry point for update loop, implementations handle time and UI there
    virtual void updateEntry( float dt ) = 0;
    //! Entry point for draw loop, implementations handle main swapchain/context and UI there
    virtual void drawEntry() = 0;

    // Optional virtual:

    //! Called before engine is constructed
    virtual void prepareSettings( AppSettings *settings )   {}
    //! Called when window size changes
    virtual void resize( const int2 &size )                 {}

    //! Override to handle keyboard events
    virtual void keyEvent( const KeyEvent &e )              {}
    //! Override to handle mouse events
    virtual void mouseEvent( const MouseEvent &e )          {}
    //! Override to return a custom app title
    virtual const char* getTitle() const                    { return "AppGlfw"; }

protected:
    std::unique_ptr<Diligent::ImGuiImplDiligent> mImGui;
    bool                               mShowUI = true; // TODO: add public api for this (move to AppBasic) instead of accessing as protected

private:
    dg::RENDER_DEVICE_TYPE chooseDefaultRenderDeviceType() const;
    bool createWindow( const AppSettings &settings, int glfwApiHint );
    bool initEngine( dg::RENDER_DEVICE_TYPE DevType );
    void initImGui();
    KeyEvent* findActiveKeyEvent( int nativeCode );
    void addOrUpdateKeyEvent( const KeyEvent &key );
    void flushOldKeyEvents();
    void loop();

	static void glfw_resizeCallback( GLFWwindow* wnd, int w, int h );
	static void glfw_keyCallback( GLFWwindow* window, int key, int scancode, int action, int mods );
    static void glfw_charCallback( GLFWwindow *glfwWindow, unsigned int codepoint );
	static void glwf_mouseButtonCallback( GLFWwindow* wnd, int button, int state, int );
	static void glfw_cursorPosCallback( GLFWwindow* wnd, double xpos, double ypos );
	static void glfw_mouseScrollCallback( GLFWwindow* wnd, double dx, double dy );
    static void glfw_errorCallback( int error, const char *description );

	friend int AppGlfwMain( int argc, const char* const* argv );

private:
    RefCntAutoPtr<dg::IRenderDevice>  mRenderDevice;
    RefCntAutoPtr<dg::IDeviceContext> mImmediateContext;
    RefCntAutoPtr<dg::ISwapChain>     mSwapChain;
    GLFWwindow*                       mWindow = nullptr;

    std::vector<KeyEvent>   mActiveKeys;
    KeyEvent                mLastKeyEvent;

    using TClock   = std::chrono::high_resolution_clock;
    using TSeconds = std::chrono::duration<float>;

    TClock::time_point mLastUpdate = {};
};

AppGlfw* CreateGLFWApp();

} // namespace juniper
