

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
        //getEngineFactory()->CreateDefaultShaderSourceStreamFactory( searchDirs.c_str(), &g->shaderSourceFactory );
        getEngineFactory()->CreateDefaultShaderSourceStreamFactory( nullptr, &g->shaderSourceFactory );
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

} // namespace juniper