#include "FXAA.h"
#include "AppGlobal.h"

#include "imgui.h"

using namespace Diligent;
namespace im = ImGui;

namespace ju { namespace aa {

FXAA::FXAA( const TEXTURE_FORMAT &colorBufferFormat )
{
    // create dynamic uniform buffers
    {
        BufferDesc CBDesc;
        CBDesc.Name           = "FXAA Constants Buffer";
        CBDesc.Size           = sizeof(mFxaaConstants);
        CBDesc.Usage          = USAGE_DYNAMIC;
        CBDesc.BindFlags      = BIND_UNIFORM_BUFFER;
        CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        app::global()->renderDevice->CreateBuffer( CBDesc, nullptr, &mConstantsBuffer );
    }

    initPipelineState( colorBufferFormat );
}

void FXAA::initPipelineState( const TEXTURE_FORMAT &colorBufferFormat )
{
    auto global = app::global();

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name         = "FXAA PSO";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    PSOCreateInfo.GraphicsPipeline.NumRenderTargets                  = 1;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                     = colorBufferFormat;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology                 = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable      = false;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = false;

    const SamplerDesc SamLinearClampDesc {
        FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR,
        TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP
    };
    const ImmutableSamplerDesc ImtblSamplers[] = {
        { SHADER_TYPE_PIXEL, "gColor", SamLinearClampDesc }
    };
    PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType  = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

    ShaderCreateInfo shaderCI;
    shaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    shaderCI.pShaderSourceStreamFactory = global->shaderSourceFactory;

    RefCntAutoPtr<IShader> vertShader;
    {
        shaderCI.Desc = { "FXAA VS", SHADER_TYPE_VERTEX, true };
        shaderCI.EntryPoint = "main";
        shaderCI.FilePath = "shaders/post/aa/fxaa.vsh";
        global->renderDevice->CreateShader( shaderCI, &vertShader );
    }

    RefCntAutoPtr<IShader> pixelShader;
    {
        shaderCI.Desc = { "FXAA PS", SHADER_TYPE_PIXEL, true };
        shaderCI.EntryPoint = "main";
        shaderCI.FilePath = "shaders/post/aa/fxaa.psh";
        global->renderDevice->CreateShader( shaderCI, &pixelShader );
    }

    PSOCreateInfo.pVS = vertShader;
    PSOCreateInfo.pPS = pixelShader;

    mPSO.Release();
    global->renderDevice->CreateGraphicsPipelineState( PSOCreateInfo, &mPSO );

    if( mPSO ) {
        auto pc = mPSO->GetStaticVariableByName( SHADER_TYPE_PIXEL, "Constants" );
        if( pc ) {
            pc->Set( mConstantsBuffer );
        }
        mPSO->CreateShaderResourceBinding( &mSRB, true );
    }
}

void FXAA::setRenderTarget( dg::ITextureView* textureView )
{
    // We need to release and create a new SRB that references new off-screen render target SRV
    // TODO: why do we need to create a new SRB? Doesn't get modified until after it is created
    // - try without
    mSRB.Release();
    mPSO->CreateShaderResourceBinding( &mSRB, true );

    auto var = mSRB->GetVariableByName( SHADER_TYPE_PIXEL, "gColor" );
    if( var ) {
        var->Set( textureView );
    }
}

void FXAA::apply( IDeviceContext* context, ITextureView *texture )
{
    context->UpdateBuffer( mConstantsBuffer, 0, sizeof( mFxaaConstants ), &mFxaaConstants, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

    // TODO: bind texture
    // TODO: run FXAA shader
}

void FXAA::updateUI()
{
    im::DragFloat( "quality subpix", &mFxaaConstants.qualitySubpix, 0.002f, 0, 1 );
    im::DragFloat( "quality edge threshold", &mFxaaConstants.qualityEdgeThreshold, 0.002f, 0, 1 );
}

}} // namespace ju::aa