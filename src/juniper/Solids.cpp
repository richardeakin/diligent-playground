// Based on Tutorial02_Cube.cpp

#include <filesystem>

#include "Solids.h"
#include "AppGlobal.h"
#include "MapHelper.hpp"
#include "GraphicsTypesX.hpp"

#include "cinder/Vector.h"

using namespace Diligent;

namespace juniper {

namespace {

using glm::vec3;

struct SceneConstants {
    //float4x4 MVP;
    mat4 MVP;
    //float4x4 normalTranform; // TODO: use mat4
    mat4 normalTranform; // TODO: use mat4
    float4   lightDirection;
};

} // anon

Solid::Solid( const Options &options )
    : mOptions( options )
{
    if( mOptions.vertPath.empty() ) {
        mOptions.vertPath = getRootAssetPath( "shaders/solids/solid.vsh" );
    }
    if( mOptions.pixelPath.empty() ) {
        mOptions.pixelPath = getRootAssetPath( "shaders/solids/solid.psh" );
    }

    // TODO: need two separate buffers here - one for SceneConstants and one for ModelConstants
    
    // create dynamic uniform buffer
    {
        BufferDesc CBDesc;
        CBDesc.Name           = "SceneConstants CB";
        CBDesc.Size           = sizeof(SceneConstants);
        CBDesc.Usage          = USAGE_DYNAMIC;
        CBDesc.BindFlags      = BIND_UNIFORM_BUFFER;
        CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        global()->renderDevice->CreateBuffer( CBDesc, nullptr, &mSceneConstants );
    }

    mOptions.staticShaderVars.push_back( { SHADER_TYPE_VERTEX, "SConstants", mSceneConstants } );
    mOptions.staticShaderVars.push_back( { SHADER_TYPE_PIXEL, "SConstants", mSceneConstants } );

    watchShadersDir();
}

Solid::~Solid()
{
}

//void Cube::setShaderResourceVar( dg::SHADER_TYPE shaderType, const dg::Char* name, dg::IDeviceObject* object )
//{
//    if( mSRB ) {
//        mSRB->GetVariableByName( shaderType, name )->Set( object );
//    }
//}

void Solid::initPipelineState()
{
    mPSO.Release();
    mSRB.Release();

    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    auto PSONameStr = mOptions.name + " PSO";
    PSOCreateInfo.PSODesc.Name = PSONameStr.c_str();
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = global()->colorBufferFormat;
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = global()->depthBufferFormat;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_BACK;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = True;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.Desc.UseCombinedTextureSamplers = true;
    ShaderCI.pShaderSourceStreamFactory = global()->shaderSourceFactory;

    RefCntAutoPtr<IShader> vertShader;
    {
        auto filePathStr = mOptions.vertPath.string();
        auto nameStr = mOptions.name + " (VS)";

        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = nameStr.c_str();
        ShaderCI.FilePath        = filePathStr.c_str();
        global()->renderDevice->CreateShader( ShaderCI, &vertShader );
    }

    RefCntAutoPtr<IShader> pixelShader;
    {
        auto filePathStr = mOptions.pixelPath.string();
        auto nameStr = mOptions.name + " (PS)";

        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = nameStr.c_str();
        ShaderCI.FilePath        = filePathStr.c_str();
        global()->renderDevice->CreateShader( ShaderCI, &pixelShader );
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

    PSOCreateInfo.pVS = vertShader;
    PSOCreateInfo.pPS = pixelShader;
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    std::vector<ShaderResourceVariableDesc>  shaderResourceDescVec;
    for( const auto &s : mOptions.shaderResourceVars ) {
        shaderResourceDescVec.push_back( s.desc );
    }
    PSOCreateInfo.PSODesc.ResourceLayout.Variables    = shaderResourceDescVec.data();
    PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = (Uint32)shaderResourceDescVec.size();

    global()->renderDevice->CreateGraphicsPipelineState( PSOCreateInfo, &mPSO );

    if( ! mPSO ) {
        LOG_ERROR_MESSAGE( __FUNCTION__, "|(", mOptions.name, ") Failed to create PSO for Solid named: ", mOptions.name );
        return;
    }

    //m_pPSO->GetStaticVariableByName( SHADER_TYPE_VERTEX, "VSConstants" )->Set( m_VSConstants );
    for( const auto &var : mOptions.staticShaderVars ) {
        auto result = mPSO->GetStaticVariableByName( var.shaderType, var.name );
        if( result ) {
            result->Set( var.object );
        }
        else {
            LOG_WARNING_MESSAGE( __FUNCTION__, "|(", mOptions.name, ") Failed to set static shader var with name: ", var.name, ", shader type: ", var.shaderType );
        }
    }

    mPSO->CreateShaderResourceBinding( &mSRB, true );

    for( const auto &var : mOptions.shaderResourceVars ) {
        auto result = mSRB->GetVariableByName( var.desc.ShaderStages, var.desc.Name );
        if( result ) {
            result->Set( var.object );
        }
        else {
            LOG_ERROR_MESSAGE( __FUNCTION__, "|(", mOptions.name, ") Failed to set shader var with name: ", var.desc.Name, ", shader type: ", var.desc.ShaderStages );
        }
    }
}

void Solid::initVertexBuffer( const std::vector<float3> &positions, const std::vector<float2> &texcoords, const std::vector<float3> &normals )
{
    VERIFY_EXPR( positions.size() == texcoords.size() );
    VERIFY_EXPR( positions.size() == normals.size() );
    VERIFY_EXPR( mOptions.components != VERTEX_COMPONENT_FLAG_NONE );

    const Uint32 totalVertexComponents =
        ( (mOptions.components & VERTEX_COMPONENT_FLAG_POSITION) ? 3 : 0 ) +
        ( (mOptions.components & VERTEX_COMPONENT_FLAG_NORMAL) ? 3 : 0 ) +
        ( (mOptions.components & VERTEX_COMPONENT_FLAG_TEXCOORD) ? 2 : 0 );

    const Uint32 numVertices = (Uint32)positions.size();

    std::vector<float> vertexData( size_t{totalVertexComponents} * numVertices );

    auto it = vertexData.begin();
    for( Uint32 v = 0; v < numVertices; ++v ) {
        if( mOptions.components & VERTEX_COMPONENT_FLAG_POSITION ) {
            const auto& Pos{positions[v]};
            *(it++) = Pos.x;
            *(it++) = Pos.y;
            *(it++) = Pos.z;
        }
        if( mOptions.components & VERTEX_COMPONENT_FLAG_NORMAL ) {
            const auto& N{normals[v]};
            *(it++) = N.x;
            *(it++) = N.y;
            *(it++) = N.z;
        }
        if( mOptions.components & VERTEX_COMPONENT_FLAG_TEXCOORD ) {
            const auto& UV{texcoords[v]};
            *(it++) = UV.x;
            *(it++) = UV.y;
        }
    }
    VERIFY_EXPR( it == vertexData.end() );

    // Create a vertex buffer that stores cube vertices
    BufferDesc bufferDesc;
    auto nameStr = mOptions.name + " vertex buffer";
    bufferDesc.Name      = nameStr.c_str();
    bufferDesc.Usage     = USAGE_IMMUTABLE;
    bufferDesc.BindFlags = BIND_VERTEX_BUFFER;
    bufferDesc.Size      = static_cast<Uint64>(vertexData.size() * sizeof(vertexData[0]));

    BufferData VBData;
    VBData.pData    = vertexData.data();
    VBData.DataSize = bufferDesc.Size;

    global()->renderDevice->CreateBuffer( bufferDesc, &VBData, &mVertexBuffer );
}

void Solid::initIndexBuffer( const std::vector<Uint32> &indices )
{
    BufferDesc bufferDesc;
    auto nameStr = mOptions.name + " index buffer";
    bufferDesc.Name      = nameStr.c_str();
    bufferDesc.Usage     = USAGE_IMMUTABLE;
    bufferDesc.BindFlags = BIND_INDEX_BUFFER;
    bufferDesc.Size      = indices.size() * sizeof(indices[0]);

    BufferData IBData;
    IBData.pData    = indices.data();
    IBData.DataSize = bufferDesc.Size;
    global()->renderDevice->CreateBuffer( bufferDesc, &IBData, &mIndexBuffer );

    mNumIndices = (Uint32)indices.size();
}

void Solid::watchShadersDir()
{
    auto shaderDir = mOptions.pixelPath.parent_path();

    if( std::filesystem::exists( shaderDir ) ) {
        LOG_INFO_MESSAGE( __FUNCTION__, "| watching assets directory: ", shaderDir );
        try {
            mShadersDirWatchHandle = std::make_unique<FileWatchType>( shaderDir.string(),
                [=](const PathType &path, const filewatch::Event change_type ) {

                    // TODO: filter out repeated events as per
                    // https://github.com/ThomasMonkman/filewatch/issues/27
                    // - for now will just mark a flag and update render loop
                    // - later will wrap in an AssetManager class that can do filtering as needed

                    // only mark if vert of frag shader was modified
                    if( mOptions.vertPath.filename() == path || mOptions.pixelPath.filename() == path ) {
                        LOG_INFO_MESSAGE( __FUNCTION__, "| \t- file event type: ", watchEventTypeToString( change_type ) , ", path: " , path );
                        mShaderAssetsMarkedDirty = true;
                    }
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

void Solid::reloadOnAssetsUpdated()
{
    LOG_INFO_MESSAGE( __FUNCTION__, "| re-initializing shader assets (", mOptions.name, ")" );

    initPipelineState();
    mShaderAssetsMarkedDirty = false;
}

void Solid::update( double deltaSeconds )
{
    if( mShaderAssetsMarkedDirty ) {
        reloadOnAssetsUpdated();
    }
}

void Solid::draw( IDeviceContext* context, const mat4 &viewProjectionMatrix, uint32_t numInstances )
{
    if( ! mPSO || ! mSRB ) {
        return;
    }

    if( GLM_FORCE_LEFT_HANDED ) {
        int blarg = 2;
    }

    static float elapsedSeconds = 0;
    elapsedSeconds += 1.0f / 60.0f;
    float aspect = 1360.0f / 991.0f;

    // Apply rotation
    //mat4 model = glm::rotate( glm::pi<float>() * 0.1f, vec3( 1.0f, 0.0f, 0.0f ) ) * glm::rotate( elapsedSeconds, vec3( 0.0f, 1.0f, 0.0f ) );
    // Camera is at (0, 0, -5) looking along the Z axis
    mat4 view = glm::lookAt( glm::vec3( 0.f, 0.0f, 5.0f ), vec3( 0.0f ), vec3( 0.0f, 1.0f, 0.0f ) );
    // Get projection matrix adjusted to the current screen orientation
    mat4 proj = glm::perspective( glm::pi<float>() / 4.0f, aspect, 0.1f, 100.f );
    // Compute world-view-projection matrix
    auto simonWorldViewProjection = proj * view * mTransform;


    // TODO: probably going to have to do some flipping here.
    // - but I think not transpose since moving to glm?
    // - flipping the order + transpose seems to do the trick, but I need to understand why
 
    // Update constant buffer
    {
        //auto mvp = mTransform * viewProjectionMatrix;
        MapHelper<SceneConstants> CBConstants( context, mSceneConstants, MAP_WRITE, MAP_FLAG_DISCARD );
        //CBConstants->MVP = glm::transpose( viewProjectionMatrix * mTransform );
        CBConstants->MVP = glm::transpose( simonWorldViewProjection ); // this works

        // We need to do inverse-transpose, but we also need to transpose the matrix before writing it to the buffer
        //CBConstants->normalTranform = mTransform.RemoveTranslation().Inverse(); // TODO: re-enable
        CBConstants->normalTranform = mat4( glm::mat3( glm::transpose( glm::inverse( mTransform ) ) ) ); // FIXME: not right yet
        CBConstants->lightDirection = mLightDirection;
    }


    // Bind vertex and index buffers
    const Uint64 offset   = 0;
    IBuffer*     pBuffs[] = { mVertexBuffer };
    context->SetVertexBuffers( 0, 1, pBuffs, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET );
    context->SetIndexBuffer( mIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

    // Set the pipeline state
    context->SetPipelineState(mPSO);
    context->CommitShaderResources( mSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

    DrawIndexedAttribs DrawAttrs;
    DrawAttrs.IndexType  = VT_UINT32;
    DrawAttrs.NumIndices = mNumIndices;
    DrawAttrs.NumInstances = numInstances;
    DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
    context->DrawIndexed( DrawAttrs );
}

// --------------------------------------------------------------------------------------------------
// Cube
// --------------------------------------------------------------------------------------------------

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

Cube::Cube( const Options &options )
    : Solid( options )
{
    if( mOptions.name.empty() ) {
        mOptions.name = "Cube";
    }

    initPipelineState();

    const std::vector<float3> positions = {
        float3{-1, -1, -1}, float3{-1, +1, -1}, float3{+1, +1, -1}, float3{+1, -1, -1}, // Bottom
        float3{-1, -1, -1}, float3{-1, -1, +1}, float3{+1, -1, +1}, float3{+1, -1, -1}, // Front
        float3{+1, -1, -1}, float3{+1, -1, +1}, float3{+1, +1, +1}, float3{+1, +1, -1}, // Right
        float3{+1, +1, -1}, float3{+1, +1, +1}, float3{-1, +1, +1}, float3{-1, +1, -1}, // Back
        float3{-1, +1, -1}, float3{-1, +1, +1}, float3{-1, -1, +1}, float3{-1, -1, -1}, // Left
        float3{-1, -1, +1}, float3{+1, -1, +1}, float3{+1, +1, +1}, float3{-1, +1, +1}  // Top
    };

    const std::vector<float2> texcoords = {
        float2{0, 1}, float2{0, 0}, float2{1, 0}, float2{1, 1}, // Bottom
        float2{0, 1}, float2{0, 0}, float2{1, 0}, float2{1, 1}, // Front
        float2{0, 1}, float2{1, 1}, float2{1, 0}, float2{0, 0}, // Right
        float2{0, 1}, float2{0, 0}, float2{1, 0}, float2{1, 1}, // Back
        float2{1, 0}, float2{0, 0}, float2{0, 1}, float2{1, 1}, // Left
        float2{1, 1}, float2{0, 1}, float2{0, 0}, float2{1, 0}  // Top
    };

    const std::vector<float3> normals = {
        float3{0, 0, -1}, float3{0, 0, -1}, float3{0, 0, -1}, float3{0, 0, -1}, // Bottom
        float3{0, -1, 0}, float3{0, -1, 0}, float3{0, -1, 0}, float3{0, -1, 0}, // Front
        float3{+1, 0, 0}, float3{+1, 0, 0}, float3{+1, 0, 0}, float3{+1, 0, 0}, // Right
        float3{0, +1, 0}, float3{0, +1, 0}, float3{0, +1, 0}, float3{0, +1, 0}, // Back
        float3{-1, 0, 0}, float3{-1, 0, 0}, float3{-1, 0, 0}, float3{-1, 0, 0}, // Left
        float3{0, 0, +1}, float3{0, 0, +1}, float3{0, 0, +1}, float3{0, 0, +1}  // Top
    };

    const std::vector<Uint32> indices = {
        2,0,1,    2,3,0,
        4,6,5,    4,7,6,
        8,10,9,   8,11,10,
        12,14,13, 12,15,14,
        16,18,17, 16,19,18,
        20,21,22, 20,22,23
    };

    initVertexBuffer( positions, texcoords, normals );
    initIndexBuffer( indices );
}

// --------------------------------------------------------------------------------------------------
// Pyramid
// --------------------------------------------------------------------------------------------------

Pyramid::Pyramid( const Options &options )
    : Solid( options )
{
    if( mOptions.name.empty() ) {
        mOptions.name = "Pyramid";
    }

    initPipelineState();

    const std::vector<float3> positions = {
        float3{ -1, -1, -1 }, float3{  0, +1,  0 }, float3{ +1, -1, -1 }, // front
        float3{ +1, -1, -1 }, float3{  0, +1,  0 }, float3{ +1, -1, +1 }, // right
        float3{ +1, -1, +1 }, float3{  0, +1,  0 }, float3{ -1, -1, +1 }, // rear
        float3{ -1, -1, +1 }, float3{  0, +1,  0 }, float3{ -1, -1, -1 }, // left
        float3{ -1, -1, -1 }, float3{ +1, -1, -1 }, float3{ -1, -1, +1 }, // bottom 1
        float3{ -1, -1, +1 }, float3{ +1, -1, -1 }, float3{ +1, -1, +1 }, // bottom 2
    };

    const std::vector<float2> texcoords = {
        float2{ 0, 0 }, float2{ 0.5f, 1 }, float2{ 1, 0 }, // front
        float2{ 0, 0 }, float2{ 0.5f, 1 }, float2{ 1, 0 }, // right
        float2{ 0, 0 }, float2{ 0.5f, 1 }, float2{ 1, 0 }, // rear
        float2{ 0, 0 }, float2{ 0.5f, 1 }, float2{ 1, 0 }, // left
        float2{ 0, 0 }, float2{ 1,    0 }, float2{ 0, 0 }, // bottom 1
        float2{ 0, 0 }, float2{ 1,    0 }, float2{ 1, 0 }, // bottom 2
    };

    // FIXME: normals need to be corrected
    const float a = 0.523599f; // 30 degrees
    const std::vector<float3> normals = {
        float3{ 0,  a / 1.0f,  -a }, float3{  0, a / 1.0f,  -a }, float3{  0,  a / 1.0f,  -a }, // front
        float3{ a,  a / 1.0f,  0 }, float3{   a,  a / 1.0f,  0 }, float3{  a,  a / 1.0f,  0 }, // right
        float3{ 0,  a / 1.0f, a }, float3{   0,  a / 1.0f, a }, float3{  0,  a / 1.0f, a }, // rear
        float3{ -a, a / 1.0f,  1 }, float3{ -a,  a / 1.0f,  1 }, float3{ -a,  a / 1.0f,  1 }, // left
        float3{ 0, -1,  0 }, float3{  0, -1,  0 }, float3{  0, -1,  0 }, // bottom 1
        float3{ 0, -1,  0 }, float3{  0, -1,  0 }, float3{  0, -1,  0 }, // bottom 2
    };

    const std::vector<Uint32> indices = {
        0, 1, 2,    // front
        3, 4, 5,    // right
        6, 7, 8,    // rear
        9, 10, 11,  // left
        12, 13, 14, // bottom 1
        15, 16, 17  // bottom 2
    };

    initVertexBuffer( positions, texcoords, normals );
    initIndexBuffer( indices );
}

} // namespace juniper
