
#include <random>
#include <vector>

#include "BasicTests.h"
#include "juniper/AppGlobal.h"
#include "juniper/Juniper.h"
#include "ShaderMacroHelper.hpp"
#include "CallbackWrapper.hpp"

#include "imgui.h"
#include "ImGuiImplDiligent.hpp"

// TODO: move this to AppBasic? Can add a flag for it..
#define LIVEPP_ENABLED 1
#if LIVEPP_ENABLED
#include "juniper/LivePP.h"
#define LPP_PATH "../../../../../tools/LivePP"
#endif

namespace juniper {

AppGlfw* CreateGLFWApp()
{
    return new BasicTests{};
}

BasicTests::~BasicTests()
{
}

// -------------------------------------------------------------------------------------------------------
// App Init
// -------------------------------------------------------------------------------------------------------

void BasicTests::prepareSettings( AppSettings *settings )
{
    settings->windowPos = { 50, 80 };
    settings->windowSize = { 1700, 1200 };
    settings->monitorIndex = 2;
    //settings->renderDeviceType = RENDER_DEVICE_TYPE_VULKAN;
}

void BasicTests::initialize()
{
    AppBasic::initialize();

#if LIVEPP_ENABLED
    ju::initLivePP( LPP_PATH );
#endif
}

// -------------------------------------------------------------------------------------------------------
// Events
// -------------------------------------------------------------------------------------------------------

static std::vector<KeyEvent> sKeyEvents;

void BasicTests::keyEvent( const KeyEvent &key )
{
    // TODO: make enum -> string converters
    // - will make a copy of cinder's app::KeyEvent, just the GLFW part
    auto state = key.getState();
    std::string stateStr = ( state == KeyEvent::State::Release ? "Release" : ( state == KeyEvent::State::Press ? "Press" : "Release" ) );
    JU_LOG_INFO( "key: ", (int)key.getKey(), ", state: ", stateStr );

    sKeyEvents.push_back( key );
    if( sKeyEvents.size() > 100 ) {
        // pop front
        sKeyEvents.erase( sKeyEvents.begin() );
    }
}

void BasicTests::mouseEvent( float2 pos )
{
    // TODO: wrap this in a new macro, as a holdover until using ci::log
    //LOG_INFO_MESSAGE( __FUNCTION__, "| mouse pos: ", pos );
    //JU_LOG_INFO( "mouse pos: ", pos );
}

// -------------------------------------------------------------------------------------------------------
// App Lifecycle
// -------------------------------------------------------------------------------------------------------

void BasicTests::update( float dt )
{
    ImGui::Text( "Hello ImGui" );

    static float s = 0.5f;
    ImGui::SliderFloat( "test slider", &s, 0, 1 );
}

void BasicTests::draw()
{
    auto* context   = getContext();
    auto* swapchain = getSwapChain();

    const float gray = 0.15f;
    clear( float4( gray, gray, gray, gray ) );

    // TODO: draw solid here
    
}

} // namespace juniper