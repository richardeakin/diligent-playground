
#include <random>
#include <vector>

#include "BasicTests.h"
#include "juniper/AppGlobal.h"
#include "juniper/Juniper.h"
#include "ShaderMacroHelper.hpp"
#include "CallbackWrapper.hpp"

#include "imgui.h"
#include "ImGuiImplDiligent.hpp"

//#include "cinder/Log.h" // TODO: get this working

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

float BackgroundGray = 0.2f;

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
#if LIVEPP_ENABLED
    ju::initLivePP( LPP_PATH );
#endif


    // make a Solid for testing 3D drawing
    {
        ju::Solid::Options options;
        options.components = ju::VERTEX_COMPONENT_FLAG_POS_NORM_UV;

        mSolid = std::make_unique<ju::Cube>( options );
        //mSolid = std::make_unique<ju::Pyramid>( options );
    }
}

// -------------------------------------------------------------------------------------------------------
// Events
// -------------------------------------------------------------------------------------------------------

namespace {

static std::vector<KeyEvent> sKeyEvents;
static std::vector<KeyEvent> sMouseEvents;

static int MaxKeyEvents = 100;
//static int MaxMouseEvents = 100;

} // anon

void BasicTests::keyEvent( const KeyEvent &e )
{
    auto state = e.getState();
    if( state == KeyEvent::State::Release ) {
        auto charStr = std::string( 1, e.getChar() );
        JU_LOG_INFO( "key (", (int)e.getKey(), ") released with char: ", charStr.c_str() );
        //CI_LOG_I( "key (" << (int)e.getKey() << ") released with char: " << charStr.c_str() ); // TODO: use
    }

    sKeyEvents.push_back( e );
    if( sKeyEvents.size() > MaxKeyEvents ) {
        // pop front
        sKeyEvents.erase( sKeyEvents.begin() );
    }
}

void BasicTests::mouseEvent( const MouseEvent &e )
{
    if( e.isScroll() ) {
        JU_LOG_INFO( "mouse pos: ", e.getPos(), ", state: ", getAsString( e.getState() ), ", scroll: ", e.getScroll() );
    }
    else if( e.isDrag() ) {
        JU_LOG_INFO( "mouse drag, pos: ", e.getPos(), ", button index: ", e.getButtonIndex() );
    }
    else if( e.getState() != MouseEvent::State::Move ) {
        JU_LOG_INFO( "mouse pos: ", e.getPos(), ", state: ", getAsString( e.getState() ), ", button index: ", e.getButtonIndex() );
    }
}

// -------------------------------------------------------------------------------------------------------
// App Lifecycle
// -------------------------------------------------------------------------------------------------------

void BasicTests::resize( const dg::int2 &size )
{
    //JU_LOG_INFO( "size: ", size );
}

// TODO: pass time through as a double always
void BasicTests::update( float deltaTime )
{
    ImGui::Text( "deltaTime: %6.3f", deltaTime );
    ImGui::SliderFloat( "background darkness", &BackgroundGray, 0, 1 );

    if( im::CollapsingHeader( "KeyEvents", ImGuiTreeNodeFlags_DefaultOpen ) ) {
        if( im::Button( "clear" ) ) {
            sKeyEvents.clear();
        }
        im::SameLine();
        im::Text( "count: %d", sKeyEvents.size() );

        static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Hideable;
        flags |= ImGuiTableFlags_ScrollY;
        flags |= ImGuiTableFlags_SizingFixedFit;

        if( im::BeginTable( "table_KeyEvents", 10, flags ) ) {
            ImGuiTableColumnFlags columnFlags = ImGuiTableColumnFlags_WidthFixed; 
            im::TableSetupScrollFreeze( 0, 1 ); // Make top row always visible
            im::TableSetupColumn( "index", columnFlags, 40 );
            im::TableSetupColumn( "char", columnFlags, 40 );
            im::TableSetupColumn( "key", columnFlags, 40 );
            im::TableSetupColumn( "native", columnFlags, 40 );
            im::TableSetupColumn( "utf32", columnFlags, 40 );
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
                    im::Text( "%c", e.getChar() );
                    im::TableSetColumnIndex( column++ );
                    im::Text( "%d", (int)e.getKey() );
                    im::TableSetColumnIndex( column++ );
                    im::Text( "%d", (int)e.getNativeKeyCode() );
                    im::TableSetColumnIndex( column++ );

                    im::Text( "%d", (int)e.getCharUtf32() );

                    im::TableSetColumnIndex( column++ );
                    im::Text( "%s", getAsString( e.getState() ) );
                    im::TableSetColumnIndex( column++ );
                    im::Text( "%d", (int)e.isShiftDown() );
                    im::TableSetColumnIndex( column++ );
                    im::Text( "%d", (int)e.isAltDown() );
                    im::TableSetColumnIndex( column++ );
                    im::Text( "%d", (int)e.isControlDown() );
                    im::TableSetColumnIndex( column++ );
                    im::Text( "%d", (int)e.isMetaDown() );
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

    const float gray = BackgroundGray;
    clear( float4( gray, gray, gray, gray ) );

    // TODO: draw solid here
    
}
