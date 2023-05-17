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

#include <chrono>
#include <vector>

#include "RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "BasicMath.hpp"

#include "GLFW/glfw3.h" // TODO: move to cpp (Key stuff to abstracted class)

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

    void Quit();

    // Interface
    virtual void prepareSettings( AppSettings *settings )  {}
    virtual bool Initialize()       = 0;
    // TODO: add resize()
    virtual void Update(float dt)   = 0;
    virtual void Draw()             = 0;

    virtual const char* getTitle() const   { return "AppGlfw"; }

    // TODO: move this to Events.h file
    enum class Key {
        Esc   = GLFW_KEY_ESCAPE,
        Space = GLFW_KEY_SPACE,
        Tab   = GLFW_KEY_TAB,

        W = GLFW_KEY_W,
        A = GLFW_KEY_A,
        S = GLFW_KEY_S,
        D = GLFW_KEY_D,

        // arrows
        Left  = GLFW_KEY_LEFT,
        Right = GLFW_KEY_RIGHT,
        Up    = GLFW_KEY_UP,
        Down  = GLFW_KEY_DOWN,

        // numpad arrows
        NP_Left  = GLFW_KEY_KP_4,
        NP_Right = GLFW_KEY_KP_6,
        NP_Up    = GLFW_KEY_KP_8,
        NP_Down  = GLFW_KEY_KP_2,

        // mouse buttons
        MB_Left   = GLFW_MOUSE_BUTTON_LEFT,
        MB_Right  = GLFW_MOUSE_BUTTON_RIGHT,
        MB_Middle = GLFW_MOUSE_BUTTON_MIDDLE,
    };
    enum class KeyState {
        Release = GLFW_RELEASE,
        Press   = GLFW_PRESS,
        Repeat  = GLFW_REPEAT,
    };
    virtual void KeyEvent( Key key, KeyState state ) = 0;

    virtual void MouseEvent( float2 pos ) = 0;

protected:
    std::unique_ptr<Diligent::ImGuiImplDiligent> mImGui;
    bool                               mShowUI = true; // TODO: add public api for this instead of accessing as protected

private:
    bool CreateWindow( const AppSettings &settings, int glfwApiHint );
    bool InitEngine( dg::RENDER_DEVICE_TYPE DevType );
    void initImGui();
    dg::RENDER_DEVICE_TYPE chooseDefaultRenderDeviceType() const;
    void Loop();
    void OnKeyEvent( Key key, KeyState state );

	static void GLFW_ResizeCallback( GLFWwindow* wnd, int w, int h );
	static void GLFW_KeyCallback( GLFWwindow* wnd, int key, int, int state, int );
	static void GLFW_MouseButtonCallback( GLFWwindow* wnd, int button, int state, int );
	static void GLFW_CursorPosCallback( GLFWwindow* wnd, double xpos, double ypos );
	static void GLFW_MouseWheelCallback( GLFWwindow* wnd, double dx, double dy );
    static void GLFW_errorCallback( int error, const char *description );

	friend int AppGlfwMain( int argc, const char* const* argv );

private:
    RefCntAutoPtr<dg::IRenderDevice>  mRenderDevice;
    RefCntAutoPtr<dg::IDeviceContext> mImmediateContext;
    RefCntAutoPtr<dg::ISwapChain>     mSwapChain;
    GLFWwindow*                       mWindow = nullptr;

    struct ActiveKey {
        Key      key;
        KeyState state;
    };
    std::vector<ActiveKey> mActiveKeys;

    using TClock   = std::chrono::high_resolution_clock;
    using TSeconds = std::chrono::duration<float>;

    TClock::time_point mLastUpdate = {};
};

AppGlfw* CreateGLFWApp();

} // namespace juniper
