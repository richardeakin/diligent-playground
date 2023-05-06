#include "FXAA.h"
#include "AppGlobal.h"

using namespace Diligent;

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
        shaderCI.Desc = { "Post process VS", SHADER_TYPE_VERTEX, true };
        shaderCI.EntryPoint = "main";
        shaderCI.FilePath = "shaders/post/post_process.vsh";
        global->renderDevice->CreateShader( shaderCI, &vertShader );
    }

    RefCntAutoPtr<IShader> pixelShader;
    {
        shaderCI.Desc = { "Post process PS", SHADER_TYPE_PIXEL, true };
        shaderCI.EntryPoint = "main";
        shaderCI.FilePath = "shaders/post/post_process.psh";
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

void FXAA::apply()
{
}

void FXAA::updateUI()
{
}

}} // namespace ju::aa