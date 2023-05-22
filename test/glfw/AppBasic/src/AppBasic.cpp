
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

static std::vector<KeyEvent> sKeyEvents;

void AppBasic::keyEvent( const KeyEvent &key )
{
    // TODO: make enum -> string converters
    // - will make a copy of cinder's app::KeyEvent, just the GLFW part
    auto state = key.getState();
    std::string stateStr = ( state == KeyEvent::State::Release ? "Release" : ( state == KeyEvent::State::Press ? "Press" : "Release" ) );
    //JU_LOG_INFO( "key: ", (int)key.getKey(), ", state: ", stateStr );

    sKeyEvents.push_back( key );
    if( sKeyEvents.size() > 100 ) {
        // pop front
        sKeyEvents.erase( sKeyEvents.begin() );
    }
}

void AppBasic::mouseEvent( float2 pos )
{
    // TODO: wrap this in a new macro, as a holdover until using ci::log
    //LOG_INFO_MESSAGE( __FUNCTION__, "| mouse pos: ", pos );
    //JU_LOG_INFO( "mouse pos: ", pos );
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