// Based on Tutorial02_Cube.cpp

#include <filesystem>

#include "Cube.h"
#include "AppGlobal.h"
#include "MapHelper.hpp"

#include "../../common/src/FileWatch.hpp"

using namespace Diligent;

namespace ju {

namespace {

std::unique_ptr<filewatch::FileWatch<std::filesystem::path>> ShadersDirWatchHandle;
bool                                                         ShaderAssetsMarkedDirty = false;

} // anon

Cube::Cube( VERTEX_COMPONENT_FLAGS components )
    : mComponents( components )
{
    initPipelineState();
    initVertexBuffer();
    initIndexBuffer();
}

void Cube::initPipelineState()
{
    auto global = app::global();

    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PSOCreateInfo.PSODesc.Name = "Cube PSO";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = global->swapChainImageDesc->ColorBufferFormat;
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = global->swapChainImageDesc->DepthBufferFormat;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_BACK;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = True;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.Desc.UseCombinedTextureSamplers = true;
    ShaderCI.pShaderSourceStreamFactory = global->shaderSourceFactory;

    // load shaders
    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Cube VS";
        ShaderCI.FilePath        = "shaders/cube/cube.vsh";
        global->renderDevice->CreateShader(ShaderCI, &pVS);
        // Create dynamic uniform buffer that will store our transformation matrix
        // Dynamic buffers can be frequently updated by the CPU
        BufferDesc CBDesc;
        CBDesc.Name           = "VS constants CB";
        CBDesc.Size           = sizeof(float4x4);
        CBDesc.Usage          = USAGE_DYNAMIC;
        CBDesc.BindFlags      = BIND_UNIFORM_BUFFER;
        CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        global->renderDevice->CreateBuffer(CBDesc, nullptr, &m_VSConstants); // TODO: move to separate (non-hotloadable) method
    }

    // Create a pixel shader
    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Cube PS";
        ShaderCI.FilePath        = "shaders/cube/cube.psh";
        global->renderDevice->CreateShader(ShaderCI, &pPS);
    }

    LayoutElement LayoutElems[] = {
        LayoutElement{0, 0, 3, VT_FLOAT32, False}, // Attrib 0 - vertex position
        LayoutElement{1, 0, 4, VT_FLOAT32, False}  // Attrib 1 - vertex color
    };
    PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
    PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = _countof(LayoutElems);

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    // Define variable type that will be used by default
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    global->renderDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);

    // Since we did not explcitly specify the type for 'Constants' variable, default
    // type (SHADER_RESOURCE_VARIABLE_TYPE_STATIC) will be used. Static variables never
    // change and are bound directly through the pipeline state object.
    m_pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_VSConstants);

    // Create a shader resource binding object and bind all static resources in it
    m_pPSO->CreateShaderResourceBinding( &m_SRB, true );
}

void Cube::initVertexBuffer()
{
    // Layout of this structure matches the one we defined in the pipeline state
    // TODO NEXT: bring over pos + texcoord + normals from TexturedCube class
    struct Vertex
    {
        float3 pos;
        float4 color;
    };

    Vertex CubeVerts[8] =
    {
        {float3(-1,-1,-1), float4(1,0,0,1)},
        {float3(-1,+1,-1), float4(0,1,0,1)},
        {float3(+1,+1,-1), float4(0,0,1,1)},
        {float3(+1,-1,-1), float4(1,1,1,1)},

        {float3(-1,-1,+1), float4(1,1,0,1)},
        {float3(-1,+1,+1), float4(0,1,1,1)},
        {float3(+1,+1,+1), float4(1,0,1,1)},
        {float3(+1,-1,+1), float4(0.2f,0.2f,0.2f,1)},
    };

    // Create a vertex buffer that stores cube vertices
    BufferDesc VertBuffDesc;
    VertBuffDesc.Name      = "Cube vertex buffer";
    VertBuffDesc.Usage     = USAGE_IMMUTABLE;
    VertBuffDesc.BindFlags = BIND_VERTEX_BUFFER;
    VertBuffDesc.Size      = sizeof(CubeVerts);
    BufferData VBData;
    VBData.pData    = CubeVerts;
    VBData.DataSize = sizeof(CubeVerts);

    app::global()->renderDevice->CreateBuffer(VertBuffDesc, &VBData, &m_CubeVertexBuffer);
}

void Cube::initIndexBuffer()
{
    Uint32 Indices[] = {
        2,0,1, 2,3,0,
        4,6,5, 4,7,6,
        0,7,4, 0,3,7,
        1,0,4, 1,4,5,
        1,5,2, 5,6,2,
        3,6,7, 3,2,6
    };

    BufferDesc IndBuffDesc;
    IndBuffDesc.Name      = "Cube index buffer";
    IndBuffDesc.Usage     = USAGE_IMMUTABLE;
    IndBuffDesc.BindFlags = BIND_INDEX_BUFFER;
    IndBuffDesc.Size      = sizeof(Indices);
    BufferData IBData;
    IBData.pData    = Indices;
    IBData.DataSize = sizeof(Indices);
    app::global()->renderDevice->CreateBuffer(IndBuffDesc, &IBData, &m_CubeIndexBuffer);

    watchShadersDir();
}

void Cube::watchShadersDir()
{
    std::filesystem::path shaderDir( "shaders/cube" );

    if( std::filesystem::exists( shaderDir ) ) {
        LOG_INFO_MESSAGE( __FUNCTION__, "| watching assets directory: ", shaderDir );
        try {
            ShadersDirWatchHandle = std::make_unique<filewatch::FileWatch<std::filesystem::path>>( shaderDir,
                [=](const std::filesystem::path &path, const filewatch::Event change_type ) {
                    //LOG_INFO_MESSAGE( __FUNCTION__, "| \t- file event type: ", watchEventTypeToString( change_type ) , ", path: " , path );
                    //ReloadOnAssetsUpdated();

                    // TODO: filter out repeated events as per
                    // https://github.com/ThomasMonkman/filewatch/issues/27
                    // - for now will just mark a flag and hope it is fine updating buffers from render loop

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

void Cube::reloadOnAssetsUpdated()
{
    LOG_INFO_MESSAGE( __FUNCTION__, "| boom" );

    m_pPSO.Release();
    m_SRB.Release();
    m_VSConstants.Release();
    initPipelineState();

    ShaderAssetsMarkedDirty = false;
}

void Cube::update( double deltaSeconds )
{
    if(ShaderAssetsMarkedDirty) {
        reloadOnAssetsUpdated();
    }
}

void Cube::render( IDeviceContext* context, const float4x4 &mvp )
{
    // Map the buffer and write current world-view-projection matrix
    {
        MapHelper<float4x4> CBConstants( context, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD );
        *CBConstants = mvp.Transpose();
    }

    // Bind vertex and index buffers
    const Uint64 offset   = 0;
    IBuffer*     pBuffs[] = {m_CubeVertexBuffer};
    context->SetVertexBuffers(0, 1, pBuffs, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    context->SetIndexBuffer(m_CubeIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Set the pipeline state
    context->SetPipelineState(m_pPSO);
    // Commit shader resources. RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode
    // makes sure that resources are transitioned to required states.
    context->CommitShaderResources( m_SRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

    DrawIndexedAttribs DrawAttrs;     // This is an indexed draw call
    DrawAttrs.IndexType  = VT_UINT32; // Index type
    DrawAttrs.NumIndices = 36;
    // Verify the state of vertex and index buffers
    DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
    context->DrawIndexed(DrawAttrs);
}


} // namespace ju