

#include "juniper/AppBasic.h"
#include "juniper/AppGlobal.h"
#include "juniper/Juniper.h"
#include "ShaderMacroHelper.hpp"
#include "CallbackWrapper.hpp"

#include "imgui.h"
#include "ImGuiImplDiligent.hpp"

#include <random>
#include <vector>

namespace juniper {

AppBasic::~AppBasic()
{
    mImGui.reset();
}

// -------------------------------------------------------------------------------------------------------
// App Init
// -------------------------------------------------------------------------------------------------------

void AppBasic::initEntry()
{
    // re-enable imgui.ini save file
    ImGui::GetIO().IniFilename = "imgui.ini";
    findRepoRootPath();

    try {
        auto g = global();
        g->renderDevice = getDevice();


        // search directories should be semi-colon separated (will likely store it locally as a vector<path>
        // TODO: make generic / portable if this stays (going to do my own fp finding and use absolute paths for now)
        //std::string searchDirs = "../../assets";
        std::string searchDirs = ( g->repoRootPath / "assets" ).string();
        getEngineFactory()->CreateDefaultShaderSourceStreamFactory( searchDirs.c_str(), &g->shaderSourceFactory );
        //getEngineFactory()->CreateDefaultShaderSourceStreamFactory( nullptr, &g->shaderSourceFactory );
        CHECK_THROW( g->shaderSourceFactory );

        g->colorBufferFormat = getSwapChain()->GetDesc().ColorBufferFormat;
        g->depthBufferFormat = getSwapChain()->GetDesc().DepthBufferFormat;

        initialize();
    }
    catch ( std::exception &exc ) {
        LOG_ERROR_MESSAGE( __FUNCTION__, "| exception caught during init, what: ", exc.what() );
    }
}

// -------------------------------------------------------------------------------------------------------
// App Lifecycle
// -------------------------------------------------------------------------------------------------------

void AppBasic::updateEntry( float dt )
{
    const float MaxDT                 = 1.0f / 30.0f; // TODO: set to 60, not sure why this is limited to 30 here

    dt = std::min( dt, MaxDT );

	if( mImGui ) {
		const auto& swapDesc = getSwapChain()->GetDesc();
		mImGui->NewFrame( swapDesc.Width, swapDesc.Height, swapDesc.PreTransform );
	}

    update( dt );
}

void AppBasic::drawEntry()
{
    auto* context   = getContext();
    auto* swapchain = getSwapChain();

    //ITextureView* rtv = swapchain->GetCurrentBackBufferRTV();
    //context->SetRenderTargets( 1, &rtv, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

    //const float gray = 0.2f;
    //const float ClearColor[4] = { gray, gray, gray, gray };
    //context->ClearRenderTarget( rtv, ClearColor, RESOURCE_STATE_TRANSITION_MODE_VERIFY );

    // TODO: this is where App impls draw() will have to get called (once this is moved to lib and subclassed
    draw();

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

// -------------------------------------------------------------------------------------------------------
// Other
// -------------------------------------------------------------------------------------------------------

void AppBasic::clear( const float4 &color )
{
    auto* context   = getContext();
    auto* swapchain = getSwapChain();

    ITextureView* rtv = swapchain->GetCurrentBackBufferRTV();
    context->SetRenderTargets( 1, &rtv, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

    context->ClearRenderTarget( swapchain->GetCurrentBackBufferRTV(), &color.r, RESOURCE_STATE_TRANSITION_MODE_VERIFY );
}

float4x4 AppBasic::getAdjustedProjectionMatrix( float fov, float nearPlane, float FarPlane ) const
{
    const auto& SCDesc = getSwapChain()->GetDesc();

    float AspectRatio = static_cast<float>(SCDesc.Width) / static_cast<float>(SCDesc.Height);
    float XScale, YScale;
    if( SCDesc.PreTransform == SURFACE_TRANSFORM_ROTATE_90 ||
        SCDesc.PreTransform == SURFACE_TRANSFORM_ROTATE_270 ||
        SCDesc.PreTransform == SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90 ||
        SCDesc.PreTransform == SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270 ) {
        // When the screen is rotated, vertical FOV becomes horizontal FOV
        XScale = 1.f / std::tan( fov / 2.0f );
        // Aspect ratio is inversed
        YScale = XScale * AspectRatio;
    }
    else {
        YScale = 1.f / std::tan( fov / 2.f );
        XScale = YScale / AspectRatio;
    }

    float4x4 Proj;
    Proj._11 = XScale;
    Proj._22 = YScale;
    Proj.SetNearFarClipPlanes( nearPlane, FarPlane, getDevice()->GetDeviceInfo().IsGLDevice() );
    return Proj;
}

float4x4 AppBasic::getSurfacePretransformMatrix( const float3& cameraViewAxis ) const
{
    const auto& SCDesc = getSwapChain()->GetDesc();
    switch (SCDesc.PreTransform)
    {
        case SURFACE_TRANSFORM_ROTATE_90:
            // The image content is rotated 90 degrees clockwise.
            return float4x4::RotationArbitrary( cameraViewAxis, -PI_F / 2.f);

        case SURFACE_TRANSFORM_ROTATE_180:
            // The image content is rotated 180 degrees clockwise.
            return float4x4::RotationArbitrary( cameraViewAxis, -PI_F);

        case SURFACE_TRANSFORM_ROTATE_270:
            // The image content is rotated 270 degrees clockwise.
            return float4x4::RotationArbitrary( cameraViewAxis, -PI_F * 3.f / 2.f);

        case SURFACE_TRANSFORM_OPTIMAL:
            UNEXPECTED("SURFACE_TRANSFORM_OPTIMAL is only valid as parameter during swap chain initialization.");
            return float4x4::Identity();

        case SURFACE_TRANSFORM_HORIZONTAL_MIRROR:
        case SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90:
        case SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180:
        case SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270:
            UNEXPECTED("Mirror transforms are not supported");
            return float4x4::Identity();

        default:
            return float4x4::Identity();
    }
}

} // namespace juniper