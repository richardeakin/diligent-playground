#include "FXAA.h"
#include "AppGlobal.h"

#include "../../common/src/FileWatch.h"

#include "imgui.h"

using namespace Diligent;
namespace im = ImGui;

namespace {

ju::FileWatchHandle     ShadersDirWatchHandle;
bool                    ShaderAssetsMarkedDirty = false;

}// anon

namespace ju { namespace aa {

FXAA::FXAA( const TEXTURE_FORMAT &colorBufferFormat )
{
    // create dynamic uniform buffers
    // TODO: want to use USAGE_DYNAMIC, but then also want to use context->UpdateBuffer() that only works with USAGE_DEFAULT.
    // - figure out what perf implications there are
    {
        BufferDesc CBDesc;
        CBDesc.Name           = "FXAA Constants Buffer";
        CBDesc.Size           = sizeof(mFxaaConstants);
        CBDesc.Usage          = USAGE_DEFAULT; // USAGE_DYNAMIC
        CBDesc.BindFlags      = BIND_UNIFORM_BUFFER;
        //CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        app::global()->renderDevice->CreateBuffer( CBDesc, nullptr, &mConstantsBuffer );
    }

    initPipelineState( colorBufferFormat );
    watchShadersDir();
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
    //PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType  = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

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
        //auto pc = mPSO->GetStaticVariableByName( SHADER_TYPE_PIXEL, "ConstantsCB" );
        //if( pc ) {
        //    pc->Set( mConstantsBuffer ); // FIXME: not getting set
        //}
        //else {
        //    LOG_WARNING_MESSAGE( "FXAA: could not set ConstantsVB variable" );
        //}
        mPSO->CreateShaderResourceBinding( &mSRB, true );
        auto var = mSRB->GetVariableByName( SHADER_TYPE_PIXEL, "ConstantsCB" );
        if( var ) {
            var->Set( mConstantsBuffer ); // FIXME: not getting set
        }
        else {
            LOG_WARNING_MESSAGE( "FXAA: could not set ConstantsVB variable" );
        }
    }
    else {
        LOG_ERROR_MESSAGE( "FXAA: null mPSO, cannot create SRB" );
    }
}

// TODO: need a way to store and reset this on shader hotloads
dg::ITextureView* sTestingTextureView = nullptr;

void FXAA::setTexture( dg::ITextureView* textureView )
{
    // We need to release and create a new SRB that references new off-screen render target SRV
    // TODO: why do we need to create a new SRB? Doesn't get modified until after it is created
    // - try without
    mSRB.Release();
    mPSO->CreateShaderResourceBinding( &mSRB, true );

    // TODO: clean this up if we have to recreate SRB, duplicate code
    auto constantsCB = mSRB->GetVariableByName( SHADER_TYPE_PIXEL, "ConstantsCB" );
    if( constantsCB ) {
        constantsCB->Set( mConstantsBuffer ); // FIXME: not getting set
    }
    else {
        LOG_WARNING_MESSAGE( "FXAA: could not set ConstantsVB variable" );
    }

    auto gColor = mSRB->GetVariableByName( SHADER_TYPE_PIXEL, "gColor" );
    if( gColor ) {
        gColor->Set( textureView );
    }

    sTestingTextureView = textureView;
}

void FXAA::watchShadersDir()
{
    std::filesystem::path shaderDir( "shaders/post/aa" );

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

void FXAA::reloadOnAssetsUpdated()
{
    LOG_INFO_MESSAGE( __FUNCTION__, "| re-initializing shader assets" );

    auto colorFormat = mPSO->GetGraphicsPipelineDesc().RTVFormats[0];

    mPSO.Release();
    mSRB.Release();
    initPipelineState( colorFormat );

    setTexture( sTestingTextureView );

    ShaderAssetsMarkedDirty = false;
}

void FXAA::apply( IDeviceContext* context, ITextureView *texture )
{
    if( ShaderAssetsMarkedDirty ) {
        reloadOnAssetsUpdated();
    }

    context->UpdateBuffer( mConstantsBuffer, 0, sizeof( mFxaaConstants ), &mFxaaConstants, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

    // TODO: bind texture
    // - actually don't think I need to, it was already done by setRenderTarget()
    
    // run FXAA
    context->SetPipelineState( mPSO );
    context->CommitShaderResources( mSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

    context->SetVertexBuffers( 0, 0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE, SET_VERTEX_BUFFERS_FLAG_RESET );
    context->SetIndexBuffer( nullptr, 0, RESOURCE_STATE_TRANSITION_MODE_NONE );

    context->Draw( DrawAttribs{ 3, DRAW_FLAG_VERIFY_ALL } );
}

void FXAA::updateUI()
{
    im::DragFloat( "quality subpix", &mFxaaConstants.qualitySubpix, 0.002f, 0, 1 );
    im::DragFloat( "quality edge threshold", &mFxaaConstants.qualityEdgeThreshold, 0.002f, 0, 1 );
}

}} // namespace ju::aa