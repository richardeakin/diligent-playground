#include "Canvas.h"

// TODO: these need to live in the same namespace / code area. currently Canvas is common but AppGlobal is not
// - will happen when I make an app layer using glfw
#include "AppGlobal.h"
#include "MapHelper.hpp"

using namespace ju;
using namespace Diligent;

Canvas::Canvas( const dg::int2 &size )
	: mSize( size )
{
	initPipelineState();
	watchShadersDir();
}

void Canvas::setSize( const int2 &size )
{
	if( mSize == size ) {
		return;
	}
	mSize = size;

	// TODO: update constants buffer with size
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
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_BACK;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.Desc.UseCombinedTextureSamplers = true;
    ShaderCI.pShaderSourceStreamFactory = global->shaderSourceFactory;

    // load shaders
    // TODO: set shader paths with constructor property
    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Canvas VS";
        ShaderCI.FilePath        = "shaders/canvas/canvas.vsh";
        global->renderDevice->CreateShader( ShaderCI, &pVS );

        // Create dynamic uniform buffer that will store our transformation matrix
        // Dynamic buffers can be frequently updated by the CPU
        // TODO: move to initialize method
        BufferDesc CBDesc;
        CBDesc.Name           = "VS constants CB";
        CBDesc.Size           = sizeof(float4);
        CBDesc.Usage          = USAGE_DYNAMIC;
        CBDesc.BindFlags      = BIND_UNIFORM_BUFFER;
        CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        global->renderDevice->CreateBuffer( CBDesc, nullptr, &mShaderConstants );
    }

    // Create a pixel shader
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
    mPSO->GetStaticVariableByName( SHADER_TYPE_VERTEX, "Constants")->Set( mShaderConstants );
    mPSO->CreateShaderResourceBinding( &mSRB, true );
}

void Canvas::watchShadersDir()
{
    // TODO
}

void Canvas::reloadOnAssetsUpdated()
{
}

void Canvas::update( double deltaSeconds )
{
}

void Canvas::render( IDeviceContext* context, const float4x4 &mvp )
{
    // update constants buffer
    {
        struct ShaderConstants {
            float2 Size;
            float2 Padding;
        };

        // Map the render target PS constant buffer and fill it in with current time
        MapHelper<ShaderConstants> CBConstants( context, mShaderConstants, MAP_WRITE, MAP_FLAG_DISCARD );
        CBConstants->Size              = float2( mSize.x, mSize.y );
    }

    context->SetPipelineState( mPSO );
    context->CommitShaderResources( mSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

    DrawAttribs drawAttribs;
    drawAttribs.NumVertices = 4;
    drawAttribs.Flags       = DRAW_FLAG_VERIFY_ALL;
    context->Draw( drawAttribs );
}
