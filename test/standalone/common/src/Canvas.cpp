#include "Canvas.h"

// TODO: these need to live in the same namespace / code area. currently Canvas is common but AppGlobal is not
// - will happen when I make an app layer using glfw
#include "AppGlobal.h"
#include "MapHelper.hpp"

#include "../../common/src/FileWatch.h"

using namespace ju;
using namespace Diligent;

namespace {

ju::FileWatchHandle     ShadersDirWatchHandle;
bool                    ShaderAssetsMarkedDirty = false;

}// anon

Canvas::Canvas( size_t sizePixelConstants )
{
    // create dynamic uniform buffers
    {
        BufferDesc CBDesc;
        CBDesc.Name           = "VertexConstants Buffer";
        CBDesc.Size           = sizeof(float4);
        CBDesc.Usage          = USAGE_DYNAMIC;
        CBDesc.BindFlags      = BIND_UNIFORM_BUFFER;
        CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        app::global()->renderDevice->CreateBuffer( CBDesc, nullptr, &mVertexConstants );
    }
    {
        BufferDesc CBDesc;
        CBDesc.Name           = "PixelConstants Buffer";
        CBDesc.Size           = sizePixelConstants;
        CBDesc.Usage          = USAGE_DYNAMIC;
        CBDesc.BindFlags      = BIND_UNIFORM_BUFFER;
        CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        app::global()->renderDevice->CreateBuffer( CBDesc, nullptr, &mPixelConstants );
    }

	initPipelineState();
	watchShadersDir();
}

void Canvas::initPipelineState()
{
    auto global = app::global();

    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PSOCreateInfo.PSODesc.Name                                  = "Canvas PSO";
    PSOCreateInfo.PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = global->swapChainImageDesc->ColorBufferFormat;
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = global->swapChainImageDesc->DepthBufferFormat;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.Desc.UseCombinedTextureSamplers = true;
    ShaderCI.pShaderSourceStreamFactory = global->shaderSourceFactory;

    // TODO: set shader paths with constructor property
    // vertex shader
    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Canvas VS";
        ShaderCI.FilePath        = "shaders/canvas/canvas.vsh";
        global->renderDevice->CreateShader( ShaderCI, &pVS );
    }

    // pixel shader
    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Canvas PS";
        ShaderCI.FilePath        = "shaders/canvas/canvas.psh";
        global->renderDevice->CreateShader( ShaderCI, &pPS );
    }

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    global->renderDevice->CreateGraphicsPipelineState( PSOCreateInfo, &mPSO );


    auto vc = mPSO->GetStaticVariableByName( SHADER_TYPE_VERTEX, "Constants");
    if( vc ) {
        vc->Set( mVertexConstants );
    }
    auto pc = mPSO->GetStaticVariableByName( SHADER_TYPE_PIXEL, "Constants" );
    if( pc ) {
        pc->Set( mPixelConstants );
    }
    mPSO->CreateShaderResourceBinding( &mSRB, true );
}

void Canvas::watchShadersDir()
{
    std::filesystem::path shaderDir( "shaders/canvas" );

    if( std::filesystem::exists( shaderDir ) ) {
        LOG_INFO_MESSAGE( __FUNCTION__, "| watching assets directory: ", shaderDir );
        try {
            ShadersDirWatchHandle = std::make_unique<FileWatchType>( shaderDir.string(),
                [=](const PathType &path, const filewatch::Event change_type ) {
                    ShaderAssetsMarkedDirty = true;
                }
            );
        }
        catch( std::system_error &exc ) {
            LOG_ERROR_MESSAGE( __FUNCTION__, "| exception caught attempting to watch directory (assets): ", shaderDir, ", what: ", exc.what() );
        }
    }
    else {
        LOG_WARNING_MESSAGE( __FUNCTION__, "| shader directory couldn't be found (not watching): ", shaderDir );
    }
}

void Canvas::reloadOnAssetsUpdated()
{
    LOG_INFO_MESSAGE( __FUNCTION__, "| re-initializing shader assets" );

    mPSO.Release();
    mSRB.Release();
    initPipelineState();

    ShaderAssetsMarkedDirty = false;
}

void Canvas::update( double deltaSeconds )
{
    if( ShaderAssetsMarkedDirty ) {
        reloadOnAssetsUpdated();
    }
}

void Canvas::render( IDeviceContext* context, const float4x4 &mvp )
{
    // update constants buffer
    {
        struct ShaderConstants {
            float2 center;
            float2 size;
        };

        MapHelper<ShaderConstants> CBConstants( context, mVertexConstants, MAP_WRITE, MAP_FLAG_DISCARD );
        CBConstants->center            = mCenter;
        CBConstants->size              = mSize;
    }

    context->SetPipelineState( mPSO );
    context->CommitShaderResources( mSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

    DrawAttribs drawAttribs;
    drawAttribs.NumVertices = 4;
    drawAttribs.Flags       = DRAW_FLAG_VERIFY_ALL;
    context->Draw( drawAttribs );
}
