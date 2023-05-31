
#include <random>
#include <vector>

#include "BasicTests.h"
#include "juniper/AppGlobal.h"
#include "juniper/Juniper.h"
#include "ShaderMacroHelper.hpp"
#include "CallbackWrapper.hpp"

#include "imgui.h"
#include "ImGuiImplDiligent.hpp"
#include "imGuIZMO.h"

#include "cinder/Log.h"
#include "glm/gtx/rotate_vector.hpp"

// TODO: move this to AppBasic? Can add a flag for it..
// TODO: set LPP_PATH as a define from cmake
#define LIVEPP_ENABLED 1
#if LIVEPP_ENABLED
#include "juniper/LivePP.h"
#define LPP_PATH "../../../../../tools/LivePP"
#endif

#define USE_CINDER_CAMERA 1

namespace im = ImGui;
using namespace Diligent;
using namespace juniper;

float BackgroundGray = 0.2f;
dg::float3      LightDir  = normalize( float3( 1, -0.5f, -0.1f ) );

bool TestSolidRotate = true;
bool DrawTestSolid = true;



#if USE_CINDER_CAMERA
using glm::vec3;
vec3 TestSolidTranslate = { 0, 0, 0 };
vec3 TestSolidScale = { 1, 1, 1 };
vec3 TestSolidLookAt = { 0, 1, 0 };
glm::mat4 ViewProjMatrix;

float CameraFov = 35;
glm::vec2 CameraClip = { 0.1f, 1000.0f };
glm::vec3 CameraEyePos = { 0, 0, 5 };
glm::vec3 CameraEyeTarget = { 0, 0, 0 };

#else
float3 TestSolidTranslate = { 0, 0, 0 };
float3 TestSolidScale = { 1, 1, 1 };
float3 TestSolidLookAt = { 0, 1, 0 };
float4x4 ViewProjMatrix;
#endif

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

void BasicTests::initCamera()
{
    mCam.lookAt( CameraEyePos, CameraEyeTarget );
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

void BasicTests::keyEvent( KeyEvent &e )
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

    if( state == KeyEvent::State::Press ) {
        mCam.keyDown( e );
    }
    else if( state == KeyEvent::State::Release ) {
        mCam.keyDown( e );
    }
}

void BasicTests::mouseEvent( MouseEvent &e )
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

    mCam.mouseEvent( e );
}

// -------------------------------------------------------------------------------------------------------
// App Lifecycle
// -------------------------------------------------------------------------------------------------------

void BasicTests::resize( const dg::int2 &size )
{
    CI_LOG_I( "size: " << size );

    vec2 windowSize = { size.x, size.y };
    mCam.perspective( CameraFov, windowSize, CameraClip[0], CameraClip[1] );
    initCamera(); // TODO: only call during initialize or button 
}

// TODO: pass time through as a double always
void BasicTests::update( float deltaTime )
{    
    updateUI();

    static double currentTime = 0; // TODO: store this on AppBasic
    currentTime += deltaTime;


#if USE_CINDER_CAMERA
    // Build a transform matrix for the test solid

    mat4 modelTransform = mat4( 1 );
    modelTransform *= glm::translate( TestSolidTranslate );

    if( TestSolidRotate )  {
        modelTransform *= glm::rotate( float(currentTime) * 0.5f, normalize( vec3( 0, 1, 0 ) ) );
        modelTransform *= glm::rotate( float(currentTime) * 0.7f, normalize( vec3( 1, 0, 0 ) ) );
    }

    modelTransform *= glm::scale( TestSolidScale );

    ViewProjMatrix = mCam.getProjectionMatrix() * mCam.getViewMatrix();
#else
    // Build a transform matrix for the test solid
    float4x4 modelTransform = float4x4::Identity();
    modelTransform *= float4x4::Scale( TestSolidScale );
    if( TestSolidRotate )  {
        modelTransform *= float4x4::RotationY( float(currentTime) * 1.0f) * float4x4::RotationX( -PI_F * 0.1f );
    }

    modelTransform *= float4x4::Translation( TestSolidTranslate );

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
#endif

    mSolid->setLightDir( LightDir );
    mSolid->setTransform( modelTransform );
    mSolid->update( deltaTime );
}

void BasicTests::updateUI()
{
    float deltaTime = 0; // TODO: get from app as a public property
    im::Text( "deltaTime: %6.3f", deltaTime );
    im::SliderFloat( "background darkness", &BackgroundGray, 0, 1 );
    im::Text( "light dir: [%0.02f, %0.02f, %0.02f]", LightDir.x, LightDir.y, LightDir.z );
    im::gizmo3D( "##LightDirection", LightDir, ImGui::GetTextLineHeight() * 5 );


    im::Separator();
    im::Text( "Test Solid" );
    im::Checkbox( "draw##test solid", &DrawTestSolid );
    im::SameLine();
    im::Checkbox( "rotate##test solid", &TestSolidRotate );
    im::DragFloat3( "translate##solid", &TestSolidTranslate.x, 0.01f );

    static bool lockDims = false;
    if( lockDims ) {
        if( im::DragFloat( "scale##solids1", &TestSolidScale.x, 0.01f, 0.001f, 1000.0f ) ) {
            //TestSolidScale = float3( TestSolidScale.x, TestSolidScale.x, TestSolidScale.x );
            TestSolidScale = vec3( TestSolidScale.x, TestSolidScale.x, TestSolidScale.x );
        }
    }
    else {
        im::DragFloat3( "scale##solid", &TestSolidScale.x, 0.01f );
    }
    im::Checkbox( "lock dims##test solid", &lockDims );

    if( im::CollapsingHeader( "Camera", ImGuiTreeNodeFlags_DefaultOpen ) ) {
        // TODO: get rid of static vars here, use the ivars on FlyCam instead
        if( im::DragFloat3( "eye pos", &CameraEyePos.x, 0.01f ) ) { 
            mCam.lookAt( CameraEyePos, CameraEyeTarget );
        }
        if( im::DragFloat3( "target", &CameraEyeTarget.x, 0.01f ) ) { 
            mCam.lookAt( CameraEyePos, CameraEyeTarget );
        }
        float moveSpeed = mCam.getMoveSpeed();
        if( im::DragFloat( "move speed", &moveSpeed, 0.01f, 0.001f, 100.0f ) ) {
            mCam.setMoveSpeed( moveSpeed );
        }
        if( im::Button( "reset" ) ) {
            initCamera();
        }

        // TODO: draw this with 3 axes (may need a quaternion, need to look at the author's examples)
        vec3 eyeOrigin = mCam.getEyeOrigin();
        if( im::DragFloat3( "origin##cam", &eyeOrigin.x, 0.01f ) ) {
            mCam.lookAt( eyeOrigin, mCam.getEyeTarget() );
        }
        auto viewDir = mCam.getWorldForward();
        float3 viewDir2 = { viewDir.x, viewDir.y, viewDir.z }; // TODO: add overloads for glm
        im::Text( "view dir: [%0.02f, %0.02f, %0.02f]", viewDir.x, viewDir.y, viewDir.z );
        im::gizmo3D( "##ViewDir", viewDir2, ImGui::GetTextLineHeight() * 5 );
    }

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
