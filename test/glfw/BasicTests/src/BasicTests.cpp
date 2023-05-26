
#include <random>
#include <vector>

#include "BasicTests.h"
#include "juniper/AppGlobal.h"
#include "juniper/Juniper.h"
#include "ShaderMacroHelper.hpp"
#include "CallbackWrapper.hpp"

#include "imgui.h"
#include "ImGuiImplDiligent.hpp"

#include "cinder/Log.h"

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
dg::float3      LightDir  = normalize( float3( 1, -0.5f, -0.1f ) );

float3 TestSolidTranslate = { 0, 0, 0 };
float3 TestSolidScale = { 1, 1, 1 };
float3 TestSolidLookAt = { 0, 1, 0 };
bool TestSolidRotate = true;
bool DrawTestSolid = true;

float4x4 ViewProjMatrix;

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

    CI_LOG_I( "blarg" );

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
static int MaxMouseEvents = 100;

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
    // TODO: pack into sMouseEvents and show in gui instead
    if( e.isScroll() ) {
        JU_LOG_INFO( "mouse pos: ", e.getPos(), ", state: ", getAsString( e.getState() ), ", scroll: ", e.getScroll() );
    }
    //else if( e.isDrag() ) {
    //    JU_LOG_INFO( "mouse drag, pos: ", e.getPos(), ", button index: ", e.getButtonIndex() );
    //}
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
    updateUI();

    static double currentTime = 0; // TODO: store this on AppBasic
    currentTime += deltaTime;

    // Build a transform matrix for the test solid
    float4x4 modelTransform = float4x4::Identity();
    modelTransform *= float4x4::Scale( TestSolidScale );
    if( TestSolidRotate )  {
        modelTransform *= float4x4::RotationY( float(currentTime) * 1.0f) * float4x4::RotationX( -PI_F * 0.1f );
    }

    modelTransform *= float4x4::Translation( TestSolidTranslate );

    mSolid->setLightDir( LightDir );
    mSolid->update( deltaTime );
    mSolid->setTransform( modelTransform );

    if( false ) {
        // TODO: use Camera once implemented
        //mViewProjMatrix = mCamera.GetViewMatrix() * mCamera.GetProjMatrix();
        //mWorldViewProjMatrix = modelTransform * mCamera.GetViewMatrix() * mCamera.GetProjMatrix();
    }
    else {
        // from samples..
        // 
        // Camera is at (0, 0, -5) looking along the Z axis
        float4x4 View = float4x4::Translation(0.f, 0.0f, 5.0f);

        // Get pretransform matrix that rotates the scene according the surface orientation
        // TODO: remove to simplify for now, this is the identity matrix on desktop
        auto SrfPreTransform = getSurfacePretransformMatrix(float3{0, 0, 1});
        //auto SrfPreTransform = float4x4::Identity();

        // Get projection matrix adjusted to the current screen orientation
        auto Proj = getAdjustedProjectionMatrix( PI_F / 4.0f, 0.1f, 100.0f );
        //auto Proj = GetAdjustedProjectionMatrix( mCamera.GetProjAttribs().FOV, mCamera.GetProjAttribs().NearClipPlane, mCamera.GetProjAttribs().FarClipPlane );

        ViewProjMatrix = View * SrfPreTransform * Proj;
        //mWorldViewProjMatrix = modelTransform * View * SrfPreTransform * Proj;
    }
}

void BasicTests::updateUI()
{
    float deltaTime = 0; // TODO: get from app 
    ImGui::Text( "deltaTime: %6.3f", deltaTime );
    ImGui::SliderFloat( "background darkness", &BackgroundGray, 0, 1 );

    im::Separator();
    im::Text( "Test Solid" );
    im::Checkbox( "draw##test solid", &DrawTestSolid );
    im::SameLine();
    im::DragFloat3( "translate##solid", &TestSolidTranslate.x, 0.01f );

    static bool lockDims = false;
    if( lockDims ) {
        if( im::DragFloat( "scale##solids1", &TestSolidScale.x, 0.01f, 0.001f, 1000.0f ) ) {
            TestSolidScale = float3( TestSolidScale.x, TestSolidScale.x, TestSolidScale.x );
        }
    }
    else {
        im::DragFloat3( "scale##solid", &TestSolidScale.x, 0.01f );
    }
    im::Checkbox( "lock dims##test solid", &lockDims );


    // TODO: move to new optional window
    if( im::CollapsingHeader( "KeyEvents" /*ImGuiTreeNodeFlags_DefaultOpen*/ ) ) {
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
    
    if( mSolid && DrawTestSolid ) {
        mSolid->draw( context, ViewProjMatrix );
    }

}
