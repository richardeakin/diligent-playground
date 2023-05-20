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

#include <random>
#include <vector>

#include "AppBasic.h"
#include "juniper/AppGlobal.h"
#include "juniper/Juniper.h"
#include "ShaderMacroHelper.hpp"
#include "CallbackWrapper.hpp"

#include "imgui.h"
#include "ImGuiImplDiligent.hpp"

namespace juniper {

AppGlfw* CreateGLFWApp()
{
    return new AppBasic{};
}

AppBasic::~AppBasic()
{
    mImGui.reset();
}

// -------------------------------------------------------------------------------------------------------
// App Init
// -------------------------------------------------------------------------------------------------------

void AppBasic::prepareSettings( AppSettings *settings )
{
    settings->windowPos = { 50, 80 };
    settings->windowSize = { 1700, 1200 };
    settings->monitorIndex = 2;
    //settings->renderDeviceType = RENDER_DEVICE_TYPE_VULKAN;
}

void AppBasic::initialize()
{
    try {
        getEngineFactory()->CreateDefaultShaderSourceStreamFactory( nullptr, &m_pShaderSourceFactory );
        CHECK_THROW( m_pShaderSourceFactory );
    }
    catch ( std::exception &exc ) {
        LOG_ERROR_MESSAGE( __FUNCTION__, "| exception caught during init, what: ", exc.what() );
    }
}

// -------------------------------------------------------------------------------------------------------
// Events
// -------------------------------------------------------------------------------------------------------

void AppBasic::keyEvent( const KeyEvent &key )
{
    // TODO: make enum -> string converters
    // - will make a copy of cinder's app::KeyEvent, just the GLFW part
    auto state = key.getState();
    std::string stateStr = ( state == KeyEvent::State::Release ? "Release" : ( state == KeyEvent::State::Press ? "Press" : "Release" ) );
    JU_LOG_INFO( "key: ", (int)key.getKey(), ", state: ", stateStr );
#if 0
    if (state == KeyState::Press || state == KeyState::Repeat) {
        switch (key)  {
        case Key::W:
        case Key::Up:
        case Key::NP_Up:
            m_Player.PendingPos.y += 1.0f;
            break;

        case Key::S:
        case Key::Down:
        case Key::NP_Down:
            m_Player.PendingPos.y -= 1.0f;
            break;

        case Key::D:
        case Key::Right:
        case Key::NP_Right:
            m_Player.PendingPos.x += 1.0f;
            break;

        case Key::A:
        case Key::Left:
        case Key::NP_Left:
            m_Player.PendingPos.x -= 1.0f;
            break;

        case Key::Space:
            m_Player.PendingPos = m_Player.FlashLightDir;
            break;

        case Key::Esc:
            Quit();
            break;

        default:
            break;
        }
    }
#endif

    //if (key == Key::MB_Left)
    //    m_Player.LMBPressed = (state != KeyState::Release);

    //// generate new map
    //if (state == KeyState::Release && key == Key::Tab)
    //    LoadNewMap();
}

void AppBasic::mouseEvent( float2 pos )
{
    // TODO: wrap this in a new macro, as a holdover until using ci::log
    //LOG_INFO_MESSAGE( __FUNCTION__, "| mouse pos: ", pos );
    JU_LOG_INFO( "mouse pos: ", pos );
}

// -------------------------------------------------------------------------------------------------------
// App Lifecycle
// -------------------------------------------------------------------------------------------------------

void AppBasic::update( float dt )
{
    const float MaxDT                 = 1.0f / 30.0f; // TODO: set to 60, not sure why this is limited to 30 here

    dt = std::min( dt, MaxDT );

	if( mImGui ) {
		const auto& swapDesc = getSwapChain()->GetDesc();
		mImGui->NewFrame( swapDesc.Width, swapDesc.Height, swapDesc.PreTransform );
		//if( m_bShowAdaptersDialog ) {
		//	UpdateAdaptersDialog();
		//}

        ImGui::Text( "Hello ImGui" );

        static float s = 0.5f;
        ImGui::SliderFloat( "test slider", &s, 0, 1 );
	}

}

void AppBasic::draw()
{
    auto* context   = getContext();
    auto* swapchain = getSwapChain();

    ITextureView* rtv = swapchain->GetCurrentBackBufferRTV();
    context->SetRenderTargets( 1, &rtv, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

    const float gray = 0.2f;
    const float ClearColor[4] = { gray, gray, gray, gray };
    context->ClearRenderTarget( rtv, ClearColor, RESOURCE_STATE_TRANSITION_MODE_VERIFY );

    // TODO: draw solid here

    if( mImGui ) {
        if( mShowUI ) {
            mImGui->Render( context );
        }
        else {
            mImGui->EndFrame();
        }
    }

    context->Flush();
    swapchain->Present();
}

} // namespace juniper