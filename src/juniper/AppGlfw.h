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
    dg::IEngineFactory* getEngineFactory()  { return mRenderDevice->GetEngineFactory(); }
    dg::IRenderDevice*  getDevice()         { return mRenderDevice; }
    dg::IDeviceContext* getContext()        { return mImmediateContext; }
    dg::ISwapChain*     getSwapChain()      { return mSwapChain; }

    void quit();

    // Interface
    virtual void prepareSettings( AppSettings *settings )  {}
    virtual void initialize()               {}
    virtual void resize( const int2 &size ) {}

    //! Entry point for update loop, implementations handle time and UI there
    virtual void updateEntry( float dt ) = 0;
    //! Entry point for draw loop, implementations handle main swapchain/context and UI there
    virtual void drawEntry() = 0;

    //! Override to handle keyboard events
    virtual void keyEvent( const KeyEvent &key ) = 0;
    //! Override to handle mouse events
    virtual void mouseEvent( float2 pos ) = 0;

    virtual const char* getTitle() const   { return "AppGlfw"; }

protected:
    std::unique_ptr<Diligent::ImGuiImplDiligent> mImGui;
    bool                               mShowUI = true; // TODO: add public api for this (move to AppBasic) instead of accessing as protected

private:
    dg::RENDER_DEVICE_TYPE chooseDefaultRenderDeviceType() const;
    bool createWindow( const AppSettings &settings, int glfwApiHint );
    bool initEngine( dg::RENDER_DEVICE_TYPE DevType );
    void initImGui();
    //void onKeyEvent( const KeyEvent &key, bool processCallback );
    void addOrUpdateKeyEvent( const KeyEvent &key );
    void flushOldKeyEvents();
    void loop();

    // TODO: fix remaining casing
	static void GLFW_ResizeCallback( GLFWwindow* wnd, int w, int h );
	static void GLFW_KeyCallback( GLFWwindow* window, int key, int scancode, int action, int mods );
    static void GLFW_CharCallback( GLFWwindow *glfwWindow, unsigned int codepoint );
	static void GLFW_MouseButtonCallback( GLFWwindow* wnd, int button, int state, int );
	static void GLFW_CursorPosCallback( GLFWwindow* wnd, double xpos, double ypos );
	static void GLFW_MouseWheelCallback( GLFWwindow* wnd, double dx, double dy );
    static void GLFW_errorCallback( int error, const char *description );

	friend int AppGlfwMain( AppGlfw *app );

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

//AppGlfw* CreateGLFWApp();

} // namespace juniper

#define JUNIPER_APP( APP )                                  \
int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR, int )      \
{                                                           \
	std::unique_ptr<AppGlfw> userApp{ new APP };            \
    return juniper::AppGlfwMain( userApp.get() );          \
}
