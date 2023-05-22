
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
// TODO: set LPP_PATH as a define from cmake
#define LIVEPP_ENABLED 1
#if LIVEPP_ENABLED
#include "juniper/LivePP.h"
#define LPP_PATH "../../../../../tools/LivePP"
#endif

namespace im = ImGui;
using namespace Diligent;
using namespace juniper;

// -------------------------------------------------------------------------------------------------------
// App Init
// -------------------------------------------------------------------------------------------------------

// a bit funky that this returns an AppGlfw
// - will address if and when I add a second app imple for other platforms like android / emscripten
juniper::AppGlfw* juniper::CreateGLFWApp()
{
    return new BasicTests{};
}

BasicTests::~BasicTests()
{
}

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

namespace {

static std::vector<KeyEvent> sKeyEvents;

static int MaxKeyEvents = 100;
static int MaxMouseEvents = 100;

} // anon

void BasicTests::keyEvent( const KeyEvent &key )
{
    // TODO: make enum -> string converters
    // - will make a copy of cinder's app::KeyEvent, just the GLFW part
    auto state = key.getState();
    std::string stateStr = ( state == KeyEvent::State::Release ? "Release" : ( state == KeyEvent::State::Press ? "Press" : "Release" ) );
    JU_LOG_INFO( "key: ", (int)key.getKey(), ", state: ", stateStr );

    sKeyEvents.push_back( key );
    if( sKeyEvents.size() > MaxKeyEvents ) {
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

// TODO: pass time through as a double always
void BasicTests::update( float deltaTime )
{
    ImGui::Text( "deltaTime: %6.3f", deltaTime );

    static float s = 0.5f;
    ImGui::SliderFloat( "test slider", &s, 0, 1 );

    if( im::CollapsingHeader( "KeyEvents", ImGuiTreeNodeFlags_DefaultOpen ) ) {
        im::Text( "count: %d", sKeyEvents.size() );

        static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Hideable;
        flags |= ImGuiTableFlags_ScrollY;
        flags |= ImGuiTableFlags_SizingFixedFit;

        if( im::BeginTable( "table_KeyEvents", 8, flags ) ) {
            ImGuiTableColumnFlags columnFlags = ImGuiTableColumnFlags_WidthFixed; 
            im::TableSetupScrollFreeze( 0, 1 ); // Make top row always visible
            im::TableSetupColumn( "index", columnFlags, 40 );
            im::TableSetupColumn( "key", columnFlags, 40 );
            im::TableSetupColumn( "char", columnFlags, 40 );
            im::TableSetupColumn( "state", columnFlags, 50 );
            im::TableSetupColumn( "shift", columnFlags, 30 );
            im::TableSetupColumn( "alt", columnFlags, 30 );
            im::TableSetupColumn( "ctrl", columnFlags, 30 );
            im::TableSetupColumn( "meta", columnFlags, 30 );
            im::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin( std::min( MaxKeyEvents, (int)sKeyEvents.size() ) );
            while( clipper.Step() ) {
                for( int row = clipper.DisplayStart; row<clipper.DisplayEnd; row++ ) {
                    im::TableNextRow();
                    int index = sKeyEvents.size() - row - 1;
                    auto e = sKeyEvents.at( index );
                    int column = 0;
                    im::TableSetColumnIndex( column++ );
                    im::Text( "%d", index );
                    im::TableSetColumnIndex( column++ );
                    im::Text( "%d", (int)e.getKey() ); // ke
                    im::TableSetColumnIndex( column++ );
                    im::Text( "%c", e.getChar() );


                    im::TableSetColumnIndex( column++ );
                    const char* stateStr = ( e.getState() == KeyEvent::State::Release ? "Release" : ( e.getState() == KeyEvent::State::Press ? "Press" : "Release" ) );
                    im::Text( "%s", stateStr );

                    im::TableSetColumnIndex( column++ );
                    im::Text( "%d", (int)e.isShiftDown() );
                    im::TableSetColumnIndex( column++ );
                    im::Text( "%d", (int)e.isAltDown() );
                    im::TableSetColumnIndex( column++ );
                    im::Text( "%d", (int)e.isControlDown() );
                    im::TableSetColumnIndex( column++ );
DSD                 im::Text( "%d", (int)e.isMetaDown() );
                }
            }
            im::EndTable();
        }

    }
}

void BasicTests::draw()
{
    auto* context   = getContext();
    auto* swapchain = getSwapChain();

    const float gray = 0.15f;
    clear( float4( gray, gray, gray, gray ) );

    // TODO: draw solid here
    
}
