// Based on Tutorial02_Cube.cpp

#include <filesystem>

#include "Cube.h"
#include "AppGlobal.h"
#include "MapHelper.hpp"
#include "GraphicsTypesX.hpp"

#include "../../common/src/FileWatch.hpp"

using namespace Diligent;

namespace ju {

namespace {

std::unique_ptr<filewatch::FileWatch<std::filesystem::path>> ShadersDirWatchHandle;
bool                                                         ShaderAssetsMarkedDirty = false;


//      (-1,+1,+1)________________(+1,+1,+1)                  Z
//               /|              /|                           |      Y
//              / |             / |                           |     /
//             /  |            /  |                           |    /
//            /   |           /   |                           |   /
//(-1,-1,+1) /____|__________/(+1,-1,+1)                      |  /
//           |    |__________|____|                           | /
//           |   /(-1,+1,-1) |    /(+1,+1,-1)                 |----------------> X
//           |  /            |   /
//           | /             |  /
//           |/              | /
//           /_______________|/
//        (-1,-1,-1)       (+1,-1,-1)
//

const Uint32 NumVertices = 4 * 6;
const Uint32 NumIndices  = 3 * 2 * 6;

const std::array<float3, NumVertices> Positions = {
    float3{-1, -1, -1}, float3{-1, +1, -1}, float3{+1, +1, -1}, float3{+1, -1, -1}, // Bottom
    float3{-1, -1, -1}, float3{-1, -1, +1}, float3{+1, -1, +1}, float3{+1, -1, -1}, // Front
    float3{+1, -1, -1}, float3{+1, -1, +1}, float3{+1, +1, +1}, float3{+1, +1, -1}, // Right
    float3{+1, +1, -1}, float3{+1, +1, +1}, float3{-1, +1, +1}, float3{-1, +1, -1}, // Back
    float3{-1, +1, -1}, float3{-1, +1, +1}, float3{-1, -1, +1}, float3{-1, -1, -1}, // Left
    float3{-1, -1, +1}, float3{+1, -1, +1}, float3{+1, +1, +1}, float3{-1, +1, +1}  // Top
};

const std::array<float2, NumVertices> Texcoords = {
    float2{0, 1}, float2{0, 0}, float2{1, 0}, float2{1, 1}, // Bottom
    float2{0, 1}, float2{0, 0}, float2{1, 0}, float2{1, 1}, // Front
    float2{0, 1}, float2{1, 1}, float2{1, 0}, float2{0, 0}, // Right
    float2{0, 1}, float2{0, 0}, float2{1, 0}, float2{1, 1}, // Back
    float2{1, 0}, float2{0, 0}, float2{0, 1}, float2{1, 1}, // Left
    float2{1, 1}, float2{0, 1}, float2{0, 0}, float2{1, 0}  // Top
};

const std::array<float3, NumVertices> Normals = {
    float3{0, 0, -1}, float3{0, 0, -1}, float3{0, 0, -1}, float3{0, 0, -1}, // Bottom
    float3{0, -1, 0}, float3{0, -1, 0}, float3{0, -1, 0}, float3{0, -1, 0}, // Front
    float3{+1, 0, 0}, float3{+1, 0, 0}, float3{+1, 0, 0}, float3{+1, 0, 0}, // Right
    float3{0, +1, 0}, float3{0, +1, 0}, float3{0, +1, 0}, float3{0, +1, 0}, // Back
    float3{-1, 0, 0}, float3{-1, 0, 0}, float3{-1, 0, 0}, float3{-1, 0, 0}, // Left
    float3{0, 0, +1}, float3{0, 0, +1}, float3{0, 0, +1}, float3{0, 0, +1}  // Top
};

const std::array<Uint32, NumIndices> Indices =
{
    2,0,1,    2,3,0,
    4,6,5,    4,7,6,
    8,10,9,   8,11,10,
    12,14,13, 12,15,14,
    16,18,17, 16,19,18,
    20,21,22, 20,22,23
};

struct VSConstants {
    float4x4 WorldViewProj;
    float4x4 NormalTranform;
    float4   LightDirection;
};

} // anon

Cube::Cube( const Options &options )
    : mOptions( options )
{
    if( mOptions.vertPath.empty() ) {
        mOptions.vertPath = "shaders/cube/cube.vsh";
    }
    if( mOptions.pixelPath.empty() ) {
        mOptions.pixelPath = "shaders/cube/cube.psh";
    }

    // create dynamic uniform buffer
    {
        BufferDesc CBDesc;
        CBDesc.Name           = "VS constants CB";
        CBDesc.Size           = sizeof(VSConstants);
        CBDesc.Usage          = USAGE_DYNAMIC;
        CBDesc.BindFlags      = BIND_UNIFORM_BUFFER;
        CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        app::global()->renderDevice->CreateBuffer( CBDesc, nullptr, &m_VSConstants );
    }

    initPipelineState();
    initVertexBuffer();
    initIndexBuffer();
    watchShadersDir();
}

// FIXME: this needs to be set from constructor options (before the SRB table is constructed
void Cube::setShaderVar( dg::SHADER_TYPE shaderType, const dg::Char* name, dg::IDeviceObject* object )
{
    if( m_pPSO ) {
        m_pPSO->GetStaticVariableByName( shaderType, name )->Set( object );
    }
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
        ShaderCI.Desc.Name       = ( ( mOptions.mLabel.empty() ? "Cube" : mOptions.mLabel ) + " (VS)" ).c_str();
        auto filePathStr = mOptions.vertPath.string();
        ShaderCI.FilePath        = filePathStr.c_str();
        global->renderDevice->CreateShader( ShaderCI, &pVS );
    }

    // Create a pixel shader
    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = ( ( mOptions.mLabel.empty() ? "Cube" : mOptions.mLabel ) + " (PS)" ).c_str();
        auto filePathStr = mOptions.pixelPath.string();
        ShaderCI.FilePath        = filePathStr.c_str();
        global->renderDevice->CreateShader( ShaderCI, &pPS );
    }

    InputLayoutDescX InputLayout;     
    Uint32 Attrib = 0;
    if( mOptions.components & VERTEX_COMPONENT_FLAG_POSITION ) {
        InputLayout.Add( Attrib++, 0u, 3u, VT_FLOAT32, False );
    }
    if( mOptions.components & VERTEX_COMPONENT_FLAG_NORMAL ) {
        InputLayout.Add( Attrib++, 0u, 3u, VT_FLOAT32, False );
    }
    if( mOptions.components & VERTEX_COMPONENT_FLAG_TEXCOORD ) {
        InputLayout.Add( Attrib++, 0u, 2u, VT_FLOAT32, False );
    }

    PSOCreateInfo.GraphicsPipeline.InputLayout = InputLayout;

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    PSOCreateInfo.PSODesc.ResourceLayout.Variables    = mOptions.shaderResourceVars.data();
    PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = mOptions.shaderResourceVars.size();

    global->renderDevice->CreateGraphicsPipelineState( PSOCreateInfo, &m_pPSO );

    m_pPSO->GetStaticVariableByName( SHADER_TYPE_VERTEX, "VSConstants" )->Set( m_VSConstants );
    m_pPSO->CreateShaderResourceBinding( &m_SRB, true );
}

void Cube::initVertexBuffer()
{
    VERIFY_EXPR( mOptions.components != VERTEX_COMPONENT_FLAG_NONE );
    const Uint32 TotalVertexComponents =
        ( (mOptions.components & VERTEX_COMPONENT_FLAG_POSITION) ? 3 : 0 ) +
        ( (mOptions.components & VERTEX_COMPONENT_FLAG_NORMAL) ? 3 : 0 ) +
        ( (mOptions.components & VERTEX_COMPONENT_FLAG_TEXCOORD) ? 2 : 0 );

    std::vector<float> VertexData(size_t{TotalVertexComponents} * NumVertices);

    auto it = VertexData.begin();
    for( Uint32 v = 0; v < NumVertices; ++v ) {
        if( mOptions.components & VERTEX_COMPONENT_FLAG_POSITION ) {
            const auto& Pos{Positions[v]};
            *(it++) = Pos.x;
            *(it++) = Pos.y;
            *(it++) = Pos.z;
        }
        if( mOptions.components & VERTEX_COMPONENT_FLAG_NORMAL ) {
            const auto& N{Normals[v]};
            *(it++) = N.x;
            *(it++) = N.y;
            *(it++) = N.z;
        }
        if( mOptions.components & VERTEX_COMPONENT_FLAG_TEXCOORD ) {
            const auto& UV{Texcoords[v]};
            *(it++) = UV.x;
            *(it++) = UV.y;
        }
    }
    VERIFY_EXPR(it == VertexData.end());

    // Create a vertex buffer that stores cube vertices
    BufferDesc VertBuffDesc;
    VertBuffDesc.Name      = "Cube vertex buffer";
    VertBuffDesc.Usage     = USAGE_IMMUTABLE;
    VertBuffDesc.BindFlags = BIND_VERTEX_BUFFER;
    VertBuffDesc.Size      = static_cast<Uint64>(VertexData.size() * sizeof(VertexData[0]));

    BufferData VBData;
    VBData.pData    = VertexData.data();
    VBData.DataSize = VertBuffDesc.Size;

    app::global()->renderDevice->CreateBuffer(VertBuffDesc, &VBData, &m_CubeVertexBuffer);
}

void Cube::initIndexBuffer()
{
    BufferDesc IndBuffDesc;
    IndBuffDesc.Name      = "Cube index buffer";
    IndBuffDesc.Usage     = USAGE_IMMUTABLE;
    IndBuffDesc.BindFlags = BIND_INDEX_BUFFER;
    IndBuffDesc.Size      = sizeof(Indices);

    BufferData IBData;
    IBData.pData    = Indices.data();
    IBData.DataSize = NumIndices * sizeof(Indices[0]);
    app::global()->renderDevice->CreateBuffer(IndBuffDesc, &IBData, &m_CubeIndexBuffer);
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
    LOG_INFO_MESSAGE( __FUNCTION__, "| re-initializing shader assets" );

    m_pPSO.Release();
    m_SRB.Release();
    initPipelineState();

    ShaderAssetsMarkedDirty = false;
}

void Cube::update( double deltaSeconds )
{
    if(ShaderAssetsMarkedDirty) {
        reloadOnAssetsUpdated();
    }
}

void Cube::render( IDeviceContext* context, const float4x4 &mvp, uint32_t numInstances )
{
    // Update constant buffer
    {
        // Map the buffer and write current world-view-projection matrix
        MapHelper<VSConstants> CBConstants( context, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        CBConstants->WorldViewProj = mvp.Transpose();

        // We need to do inverse-transpose, but we also need to transpose the matrix
        // before writing it to the buffer
        auto NormalMatrix          = mTransform.RemoveTranslation().Inverse();
        CBConstants->NormalTranform = NormalMatrix;
        CBConstants->LightDirection = mLightDirection;
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
    DrawAttrs.NumInstances = numInstances;
    // Verify the state of vertex and index buffers
    DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
    context->DrawIndexed(DrawAttrs);
}


} // namespace ju