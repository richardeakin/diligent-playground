
// disable annoying int to float conversion warnings
#pragma warning( disable : 4244 )

#include "ComputeParticles.hpp"
#include "BasicMath.hpp"
#include "MapHelper.hpp"
#include "imgui.h"
#include "imGuIZMO.h"
#include "ShaderMacroHelper.hpp"

#include "AppGlobal.h"
#include "../../common/src/FileWatch.h"
#include "../../common/src/LivePP.h"

#define LPP_PATH "../../../../../tools/LivePP"

#include <filesystem>
#include <random>

using namespace Diligent;
using namespace ju;
namespace im = ImGui;

Diligent::SampleBase* Diligent::CreateSample()
{
    return new ComputeParticles();
}

namespace {

struct ParticleAttribs {
    float3 pos;
    float  padding0;
    float3 newPos;
    float  padding1;

    float3 vel;
    float  padding2;
    float3 newVel;
    float  padding3;

    float3  accel;
    float   padding4;
    float3  newAccel;
    float   padding5;

    float size              = 0;
    float temperature       = 0;
    int   numInteractions   = 0;
    float padding6          = 0;
};

// TODO: rather than duplicating all these vars as member vars, would be easier to keep this struct in
// the class and then pass in our copy of ParticleConstants to MapHelper (or use map fns directly)
// - do it the same way that PostProcessConstants is handled
// - consider storing all structures in assets/shaders/structures.fxh
struct ParticleConstants {
    float4x4 viewProj;

    uint    numParticles;
    float   time;
    float   deltaTime;
    float   separation;

    float   scale;
    int3    gridSize;

    float2  speedMinMax;
    float   alignment;
    float   cohesion;

    float   separationDist;
    float   alignmentDist;
    float   cohesionDist;
    float   padding1;
};

struct BackgroundPixelConstants {
    float4x4 inverseViewProj;

    float3 camPos;
    float padding0;
    float3 camDir;
    float padding1;
    float2 resolution;
    float padding2;
    float padding3;
};


bool UseFirstPersonCamera = true;
float3 TestSolidTranslate = { 0, 0, 0 };
float3 TestSolidScale = { 0.4f, 1, 0.4f };
float3 TestSolidLookAt = { 0, 1, 0 };
bool TestSolidUseLookAt = true;
QuaternionF TestSolidRotation = {0, 0, 0, 1};

float CameraRotationSpeed = 0.005f;
float CameraMoveSpeed = 8.0f;
float2 CameraSpeedUp = { 0.2f, 10.0f }; // speed multipliers when {shift, ctrl} is down

dg::float3      LightDir  = normalize( float3( 1, -0.5f, -0.1f ) );

ju::FileWatchHandle     ShadersDirWatchHandle, ShadersDirWatchHandle2;
bool                    ParticleShaderAssetsMarkedDirty = false;
bool                    PostShaderAssetsMarkedDirty = false;

std::vector<ParticleAttribs> DebugParticleAttribsData;
std::vector<int> DebugParticleListsData;
std::vector<int> DebugParticleListsHeadData;
static bool DebugShowParticleAttribsWindow = true;
static bool DebugShowParticleListssWindow = true;

// returns a quaternion that rotates vector a to vector b
QuaternionF GetRotationQuat( const float3 &a, const float3 &b, const float3 &up )
{   
    //ASSERT_VECTOR_NORMALIZED(a);
    //ASSERT_VECTOR_NORMALIZED(b);

	float dotAB = dot( a, b );
	// test for dot = -1
	if( fabsf( dotAB - (-1.0f) ) < 0.000001f ) {
		// vector a and b point exactly in the opposite direction, 
		// so it is a 180 degrees turn around the up-axis
		return QuaternionF( up.x, up.y, up.z, PI_F );
	}
	// test for dot = 1
    if( fabsf( dotAB - (1.0f) ) < 0.000001f ) {
        // vector a and b point exactly in the same direction
		// so we return the identity quaternion
		return QuaternionF( 0.0f, 0.0f, 0.0f, 1.0f );
	}

	float rotAngle = acos( dotAB );
	float3 rotAxis = cross( a, b );
	rotAxis = normalize( rotAxis );
    return QuaternionF::RotationFromAxisAngle( rotAxis, rotAngle );
}

} // anon

void ComputeParticles::ModifyEngineInitInfo( const ModifyEngineInitInfoAttribs& Attribs )
{
    SampleBase::ModifyEngineInitInfo( Attribs );

    Attribs.EngineCI.Features.ComputeShaders    = DEVICE_FEATURE_STATE_ENABLED;
    Attribs.EngineCI.Features.TimestampQueries  = DEVICE_FEATURE_STATE_OPTIONAL;
    Attribs.EngineCI.Features.DurationQueries   = DEVICE_FEATURE_STATE_OPTIONAL;

    Attribs.SCDesc.DepthBufferFormat = TEX_FORMAT_UNKNOWN; // we're rendering to offscreen buffers so no need for depth buffer on the swap chain
}

void ComputeParticles::Initialize( const SampleInitInfo& InitInfo )
{
    ju::initLivePP( LPP_PATH );

    SampleBase::Initialize( InitInfo );

    // re-enable imgui.ini save file
    im::GetIO().IniFilename = "imgui.ini";

    auto global = app::global();
    global->renderDevice = m_pDevice;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory( nullptr, &global->shaderSourceFactory );

    // set texture formats (used when creating render targets)
	if( m_pDevice->GetTextureFormatInfoExt( TEX_FORMAT_D32_FLOAT ).BindFlags & BIND_DEPTH_STENCIL ) {
		global->depthBufferFormat = TEX_FORMAT_D32_FLOAT;
    }
	else if( m_pDevice->GetTextureFormatInfoExt( TEX_FORMAT_D24_UNORM_S8_UINT ).BindFlags & BIND_DEPTH_STENCIL ) {
		global->depthBufferFormat = TEX_FORMAT_D24_UNORM_S8_UINT;
    }

	// Use HDR format if supported.
	constexpr auto RTFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
	if( ( m_pDevice->GetTextureFormatInfoExt( TEX_FORMAT_RGBA16_FLOAT ).BindFlags & RTFlags ) == RTFlags ) {
        global->colorBufferFormat = TEX_FORMAT_RGBA16_FLOAT;
    }

    initConsantBuffers();
    initRenderParticlePSO();
    initUpdateParticlePSO();
    initParticleBuffers();
    initPostProcessPSO();


    mBackgroundCanvas = std::make_unique<ju::Canvas>( sizeof(BackgroundPixelConstants) );
    initSolids();
    initCamera();

    mFXAA = std::make_unique<ju::aa::FXAA>( m_pSwapChain->GetDesc().ColorBufferFormat );
    mProfiler = std::make_unique<ju::Profiler>( m_pDevice );

    watchShadersDir();
}

void ComputeParticles::initRenderParticlePSO()
{
    mRenderParticlePSO.Release();

    GraphicsPipelineStateCreateInfo psoCreateInfo;

    psoCreateInfo.PSODesc.Name = "Render particles PSO";
    psoCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    psoCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    psoCreateInfo.GraphicsPipeline.RTVFormats[0]                = app::global()->colorBufferFormat;
    psoCreateInfo.GraphicsPipeline.DSVFormat                    = app::global()->depthBufferFormat;
    psoCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

    psoCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    psoCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = false;

    auto& blendDesc = psoCreateInfo.GraphicsPipeline.BlendDesc;
    blendDesc.RenderTargets[0].BlendEnable = True;
    blendDesc.RenderTargets[0].SrcBlend    = BLEND_FACTOR_SRC_ALPHA;
    blendDesc.RenderTargets[0].DestBlend   = BLEND_FACTOR_INV_SRC_ALPHA;

    ShaderCreateInfo shaderCI;
    shaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    shaderCI.Desc.UseCombinedTextureSamplers = true; // needed for OpenGL support
    // TODO: enable these lines and see if any of my hlsl compile errors are fixed
    //ShaderCI.ShaderCompiler = SHADER_COMPILER_DXC; // use modern HLSL compiler
    //ShaderCI.HLSLVersion    = {6, 3};

    // TODO: use global one instead
    RefCntAutoPtr<IShaderSourceInputStreamFactory> shaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory( nullptr, &shaderSourceFactory );
    shaderCI.pShaderSourceStreamFactory = shaderSourceFactory;

    RefCntAutoPtr<IShader> particleSpriteVS;
    {
        shaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        shaderCI.EntryPoint      = "main";
        shaderCI.Desc.Name       = "Particle VS";
        shaderCI.FilePath        = "shaders/particles/particle_sprite.vsh";
        m_pDevice->CreateShader( shaderCI, &particleSpriteVS );
    }

    // Create particle pixel shader
    RefCntAutoPtr<IShader> particleSpritePS;
    {
        shaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        shaderCI.EntryPoint      = "main";
        shaderCI.Desc.Name       = "Particle PS";
        shaderCI.FilePath        = "shaders/particles/particle_sprite.psh";

        m_pDevice->CreateShader( shaderCI, &particleSpritePS );
    }

    psoCreateInfo.pVS = particleSpriteVS;
    psoCreateInfo.pPS = particleSpritePS;
    psoCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    ShaderResourceVariableDesc vars[] = {
        { SHADER_TYPE_VERTEX, "Particles", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE }
    };

    psoCreateInfo.PSODesc.ResourceLayout.Variables    = vars;
    psoCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(vars);

    m_pDevice->CreateGraphicsPipelineState( psoCreateInfo, &mRenderParticlePSO );
    if( mRenderParticlePSO ) {
        auto vc = mRenderParticlePSO->GetStaticVariableByName( SHADER_TYPE_VERTEX, "Constants" );
        if( vc ) {
            vc->Set( mParticleConstants );
        }
    }
}

void ComputeParticles::initUpdateParticlePSO()
{
    mResetParticleListsPSO.Release();
    mMoveParticlesPSO.Release();
    mInteractParticlesPSO.Release();

    // TODO: update variable names and cleanup unnecessary comments

    ShaderCreateInfo shaderCI;
    shaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    shaderCI.Desc.UseCombinedTextureSamplers = true; // for OpenGL

    // TODO: use global one instead
    RefCntAutoPtr<IShaderSourceInputStreamFactory> shaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory( nullptr, &shaderSourceFactory );
    shaderCI.pShaderSourceStreamFactory = shaderSourceFactory;

    ShaderMacroHelper shaderMacros;
    shaderMacros.AddShaderMacro( "THREAD_GROUP_SIZE", mThreadGroupSize );
    shaderMacros.Finalize();

    RefCntAutoPtr<IShader> resetParticleListsCS;
    {
        shaderCI.Desc.ShaderType = SHADER_TYPE_COMPUTE;
        shaderCI.EntryPoint      = "main";
        shaderCI.Desc.Name       = "Reset Particle lists CS";
        shaderCI.FilePath        = "shaders/particles/reset_particle_lists.csh";
        shaderCI.Macros          = shaderMacros;
        m_pDevice->CreateShader( shaderCI, &resetParticleListsCS );
    }

    RefCntAutoPtr<IShader> moveParticlesCS;
    {
        shaderCI.Desc.ShaderType = SHADER_TYPE_COMPUTE;
        shaderCI.EntryPoint      = "main";
        shaderCI.Desc.Name       = "Move Particles CS";
        shaderCI.FilePath        = "shaders/particles/move_particles.csh";
        shaderCI.Macros          = shaderMacros;
        m_pDevice->CreateShader( shaderCI, &moveParticlesCS );
    }

    RefCntAutoPtr<IShader> interactParticlesCS;
    {
        shaderCI.Desc.ShaderType = SHADER_TYPE_COMPUTE;
        shaderCI.EntryPoint      = "main";
        shaderCI.Desc.Name       = "Interact Particles CS";
        shaderCI.FilePath        = "shaders/particles/interact_particles.csh";
        shaderCI.Macros          = shaderMacros;
        m_pDevice->CreateShader( shaderCI, &interactParticlesCS );
    }

    ComputePipelineStateCreateInfo psoCI;
    PipelineStateDesc&             psoDesc = psoCI.PSODesc;

    // init compute pipeline
    psoDesc.PipelineType = PIPELINE_TYPE_COMPUTE;
    psoDesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
    ShaderResourceVariableDesc shaderVars[] = {
        { SHADER_TYPE_COMPUTE, "Constants", SHADER_RESOURCE_VARIABLE_TYPE_STATIC }
    };
    psoDesc.ResourceLayout.Variables    = shaderVars;
    psoDesc.ResourceLayout.NumVariables = _countof(shaderVars);

    psoDesc.Name = "Reset particle lists PSO";
    psoCI.pCS = resetParticleListsCS;
    m_pDevice->CreateComputePipelineState( psoCI, &mResetParticleListsPSO );
    if( auto var = mResetParticleListsPSO->GetStaticVariableByName( SHADER_TYPE_COMPUTE, "Constants" ) ) {
        var->Set( mParticleConstants );
    }

    psoDesc.Name = "Move particles PSO";
    psoCI.pCS = moveParticlesCS;
    m_pDevice->CreateComputePipelineState( psoCI, &mMoveParticlesPSO );
    if( mMoveParticlesPSO ) {
        if( auto var = mMoveParticlesPSO->GetStaticVariableByName( SHADER_TYPE_COMPUTE, "Constants" ) ) {
            var->Set( mParticleConstants );
        }
    }

    psoDesc.Name = "Interact particles PSO";
    psoCI.pCS = interactParticlesCS;
    m_pDevice->CreateComputePipelineState( psoCI, &mInteractParticlesPSO );
    if( mInteractParticlesPSO ) {
        mInteractParticlesPSO->GetStaticVariableByName( SHADER_TYPE_COMPUTE, "Constants" )->Set( mParticleConstants );
    }
}

void ComputeParticles::initSolids()
{
    // make a Solid for testing
    {
        ju::Solid::Options options;
        options.components = ju::VERTEX_COMPONENT_FLAG_POS_NORM_UV;

        if( mParticleType == ParticleType::Pyramid ) {
            mTestSolid = std::make_unique<ju::Pyramid>( options );
        }
        else {
            mTestSolid = std::make_unique<ju::Cube>( options );
        }
    }

    // make the Solid that will get used for instanced drawing of particles
    {
        IBufferView* particleAttribsBufferSRV = mParticleAttribsBuffer->GetDefaultView( BUFFER_VIEW_SHADER_RESOURCE );

        ju::Solid::Options options;
        options.components = ju::VERTEX_COMPONENT_FLAG_POS_NORM_UV;
        options.vertPath = "shaders/particles/particle_solid.vsh";
        options.pixelPath = "shaders/particles/particle_solid.psh";
        options.name = "Particle Solid";
        options.shaderResourceVars.push_back( { { SHADER_TYPE_VERTEX, "Particles", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE }, particleAttribsBufferSRV } );
        options.staticShaderVars.push_back( { SHADER_TYPE_VERTEX, "PConstants", mParticleConstants } );
        options.staticShaderVars.push_back( { SHADER_TYPE_PIXEL, "PConstants", mParticleConstants } );

        if( mParticleType == ParticleType::Cube ) {
            mParticleSolid = std::make_unique<ju::Cube>( options );
        }
        else {
            mParticleSolid = std::make_unique<ju::Pyramid>( options );
        }
    }
}

void ComputeParticles::initParticleBuffers()
{
    mParticleAttribsBuffer.Release();
    mParticleListHeadsBuffer.Release();
    mParticleListsBuffer.Release();
#if DEBUG_PARTICLE_BUFFERS
    mParticleAttribsStaging.Release();
    mParticleListsStaging.Release();
    mParticleListsHeadStaging.Release();
    mFenceParticleAttribsAvailable.Release();
#endif

    BufferDesc BuffDesc;
    BuffDesc.Name              = "Particle attribs buffer";
    BuffDesc.Usage             = USAGE_DEFAULT;
    BuffDesc.BindFlags         = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
    BuffDesc.Mode              = BUFFER_MODE_STRUCTURED;
    BuffDesc.ElementByteStride = sizeof(ParticleAttribs);
    BuffDesc.Size              = sizeof(ParticleAttribs) * mNumParticles;

    std::vector<ParticleAttribs> ParticleData( mNumParticles );

    std::mt19937 gen; // Standard mersenne_twister_engine. Use default seed
                      // to generate consistent distribution.

    std::uniform_real_distribution<float> pos_distr(-1.f, +1.f);
    std::uniform_real_distribution<float> size_distr(0.5f, 1.f);

    // TODO: make size based on a member var + variation
    // - don't need scale at that point, except if I'm using to shape the 
    constexpr float fMaxParticleSize = 0.05f;
    float           fSize            = 0.7f / std::sqrt( static_cast<float>( mNumParticles ) );
    fSize                            = std::min( fMaxParticleSize, fSize );
    for ( auto &particle : ParticleData ) {
        particle.newPos.x   = pos_distr(gen);
        particle.newPos.y   = pos_distr(gen);
        particle.newPos.z   = pos_distr(gen);
        particle.newVel.x = pos_distr(gen) * fSize * 5.f;
        particle.newVel.y = pos_distr(gen) * fSize * 5.f;
        particle.newVel.z = pos_distr(gen) * fSize * 5.f;
        particle.size       = fSize * size_distr(gen);
    }

    BufferData VBData;
    VBData.pData    = ParticleData.data();
    VBData.DataSize = sizeof(ParticleAttribs) * static_cast<Uint32>( ParticleData.size() );
    m_pDevice->CreateBuffer( BuffDesc, &VBData, &mParticleAttribsBuffer );
    IBufferView* particleAttribsBufferSRV = mParticleAttribsBuffer->GetDefaultView( BUFFER_VIEW_SHADER_RESOURCE );
    IBufferView* particleAttribsBufferUAV = mParticleAttribsBuffer->GetDefaultView( BUFFER_VIEW_UNORDERED_ACCESS );

    BuffDesc.ElementByteStride = sizeof(int);
    BuffDesc.Mode              = BUFFER_MODE_FORMATTED;
    BuffDesc.Size              = Uint64{BuffDesc.ElementByteStride} * static_cast<Uint64>( mNumParticles );
    BuffDesc.BindFlags         = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
    m_pDevice->CreateBuffer( BuffDesc, nullptr, &mParticleListHeadsBuffer );
    m_pDevice->CreateBuffer( BuffDesc, nullptr, &mParticleListsBuffer );
    RefCntAutoPtr<IBufferView> particleListHeadsBufferUAV;
    RefCntAutoPtr<IBufferView> particleListsBufferUAV;
    RefCntAutoPtr<IBufferView> particleListHeadsBufferSRV;
    RefCntAutoPtr<IBufferView> particleListsBufferSRV;
    {
        BufferViewDesc ViewDesc;
        ViewDesc.ViewType             = BUFFER_VIEW_UNORDERED_ACCESS;
        ViewDesc.Format.ValueType     = VT_INT32;
        ViewDesc.Format.NumComponents = 1;
        mParticleListHeadsBuffer->CreateView( ViewDesc, &particleListHeadsBufferUAV );
        mParticleListsBuffer->CreateView( ViewDesc, &particleListsBufferUAV );

        ViewDesc.ViewType = BUFFER_VIEW_SHADER_RESOURCE;
        mParticleListHeadsBuffer->CreateView( ViewDesc, &particleListHeadsBufferSRV );
        mParticleListsBuffer->CreateView( ViewDesc, &particleListsBufferSRV );
    }

#if DEBUG_PARTICLE_BUFFERS
    // make a staging buffer to read back
    {
        BufferDesc bufferDescStaging;

        bufferDescStaging.Name           = "ParticleAttribs staging buffer";
        bufferDescStaging.Usage          = USAGE_STAGING;
        bufferDescStaging.BindFlags      = BIND_NONE;
        bufferDescStaging.Mode           = BUFFER_MODE_UNDEFINED;
        bufferDescStaging.CPUAccessFlags = CPU_ACCESS_READ;
        bufferDescStaging.Size           = sizeof(ParticleAttribs) * mNumParticles;
        m_pDevice->CreateBuffer( bufferDescStaging, nullptr, &mParticleAttribsStaging );
        VERIFY_EXPR( mParticleAttribsStaging != nullptr );

        bufferDescStaging.Name           = "ParticleLists staging buffer";
        bufferDescStaging.Size           = sizeof(int) * mNumParticles;
        m_pDevice->CreateBuffer( bufferDescStaging, nullptr, &mParticleListsStaging );
        VERIFY_EXPR( mParticleListsStaging != nullptr );

        bufferDescStaging.Name           = "ParticleListsHead staging buffer";
        bufferDescStaging.Size           = sizeof(int) * mNumParticles;
        m_pDevice->CreateBuffer( bufferDescStaging, nullptr, &mParticleListsHeadStaging );
        VERIFY_EXPR( mParticleListsStaging != nullptr );

        FenceDesc fenceDesc;
        fenceDesc.Name = "ParticleAttribs available";
        fenceDesc.Type = FENCE_TYPE_GENERAL; // TODO: is this necessary? MeshShaders tutorial did not use it
        m_pDevice->CreateFence( fenceDesc, &mFenceParticleAttribsAvailable );
    }
#endif

    if( mResetParticleListsPSO ) {
        mResetParticleListsSRB.Release();
        mResetParticleListsPSO->CreateShaderResourceBinding( &mResetParticleListsSRB, true );
        mResetParticleListsSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "ParticleListHead")->Set( particleListHeadsBufferUAV );
    }
    if( mRenderParticlePSO ) {
        mRenderParticleSRB.Release();
        mRenderParticlePSO->CreateShaderResourceBinding( &mRenderParticleSRB, true );
        mRenderParticleSRB->GetVariableByName( SHADER_TYPE_VERTEX, "Particles" )->Set( particleAttribsBufferSRV );
    }
    if( mMoveParticlesPSO ) {
        mMoveParticlesSRB.Release();
        mMoveParticlesPSO->CreateShaderResourceBinding( &mMoveParticlesSRB, true );
        mMoveParticlesSRB->GetVariableByName( SHADER_TYPE_COMPUTE, "Particles" )->Set( particleAttribsBufferUAV );
        mMoveParticlesSRB->GetVariableByName( SHADER_TYPE_COMPUTE, "ParticleListHead" )->Set( particleListHeadsBufferUAV );
        mMoveParticlesSRB->GetVariableByName( SHADER_TYPE_COMPUTE, "ParticleLists" )->Set( particleListsBufferUAV );
    }
    if( mInteractParticlesPSO ) {
        mInteractParticlesSRB.Release();
        mInteractParticlesPSO->CreateShaderResourceBinding( &mInteractParticlesSRB, true );
        mInteractParticlesSRB->GetVariableByName( SHADER_TYPE_COMPUTE, "Particles" )->Set( particleAttribsBufferUAV );

        auto listHead =  mInteractParticlesSRB->GetVariableByName( SHADER_TYPE_COMPUTE, "ParticleListHead" );
        if( listHead ) {
            listHead->Set( particleListHeadsBufferSRV );
        }
        auto lists = mInteractParticlesSRB->GetVariableByName( SHADER_TYPE_COMPUTE, "ParticleLists" );
        if( lists ) {
            lists->Set( particleListsBufferSRV );
        }
    }
}

void ComputeParticles::initConsantBuffers()
{
    // ParticleConstants
    {
        BufferDesc BuffDesc;
        BuffDesc.Name           = "ParticleConstants buffer";
        BuffDesc.Usage          = USAGE_DYNAMIC;
        BuffDesc.BindFlags      = BIND_UNIFORM_BUFFER;
        BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        BuffDesc.Size           = sizeof(ParticleConstants);
        m_pDevice->CreateBuffer( BuffDesc, nullptr, &mParticleConstants );
    }

    // PostProcessConstants
    {
        BufferDesc BuffDesc;
        BuffDesc.BindFlags            = BIND_UNIFORM_BUFFER;
        BuffDesc.Usage                = USAGE_DEFAULT;
        BuffDesc.Size                 = sizeof(PostProcessConstants);
        BuffDesc.ImmediateContextMask = (Uint64{1} << m_pImmediateContext->GetDesc().ContextId);
        BuffDesc.Name                 = "PostProcessConstants buffer";
        m_pDevice->CreateBuffer( BuffDesc, nullptr, &mPostProcessConstantsBuffer );
    }
}

void ComputeParticles::initCamera()
{
    mCamera.SetPos( float3{ 0, 0, -4 } );
    mCamera.SetLookAt( float3{ 0, 0, 1 } );
    mCamera.SetRotationSpeed( CameraRotationSpeed );
    mCamera.SetMoveSpeed( CameraMoveSpeed );
    mCamera.SetSpeedUpScales( CameraSpeedUp.x, CameraSpeedUp.y );
}

void ComputeParticles::watchShadersDir()
{
    // particles
    {
        std::filesystem::path shaderDir( "shaders/particles" );

        if( std::filesystem::exists( shaderDir ) ) {
            LOG_INFO_MESSAGE( __FUNCTION__, "| watching assets directory: ", shaderDir );
            try {
                ShadersDirWatchHandle = std::make_unique<FileWatchType>( shaderDir.string(),
                    [=](const PathType &path, const filewatch::Event change_type ) {

                        // make a list of files we actually want to update if changed and check that here
                        const static std::vector<PathType> checkFilenames = {
                            "interact_particles.csh",
                            "move_particles.csh",
                            "reset_particles.csh",
                            "particle_sprite.vsh",
                            "particle_sprite.psh",
                            "particle.fxh",
                            "structures.fxh"
                        };

                        for( const auto &p : checkFilenames ) {
                            if( p == path ) {
                                LOG_INFO_MESSAGE( __FUNCTION__, "| \t- file event type: ", watchEventTypeToString( change_type ) , ", path: " , path );
                                ParticleShaderAssetsMarkedDirty = true;
                            }
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

    // post
    {
        std::filesystem::path shaderDir( "shaders/post" );

        if( std::filesystem::exists( shaderDir ) ) {
            LOG_INFO_MESSAGE( __FUNCTION__, "| watching assets directory: ", shaderDir );
            try {
                ShadersDirWatchHandle2 = std::make_unique<FileWatchType>( shaderDir.string(),
                    [=](const PathType &path, const filewatch::Event change_type ) {

                        // make a list of files we actually want to update if changed and check that here
                        const static std::vector<PathType> checkFilenames = {
                            "post_process.vsh",
                            "post_process.psh",
                        };

                        for( const auto &p : checkFilenames ) {
                            if( p == path ) {
                                LOG_INFO_MESSAGE( __FUNCTION__, "| \t- file event type: ", watchEventTypeToString( change_type ) , ", path: " , path );
                                PostShaderAssetsMarkedDirty = true;
                            }
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

}

void ComputeParticles::checkReloadOnAssetsUpdated()
{
    if( ParticleShaderAssetsMarkedDirty ) {
        LOG_INFO_MESSAGE( __FUNCTION__, "| re-initializing Particle shader assets" );

        initRenderParticlePSO();
        initUpdateParticlePSO();

        ParticleShaderAssetsMarkedDirty = false;
    }

    if( PostShaderAssetsMarkedDirty ) {
        LOG_INFO_MESSAGE( __FUNCTION__, "| re-initializing Post Process shader assets" );

        initPostProcessPSO();

        PostShaderAssetsMarkedDirty = false;
    }
}

void ComputeParticles::WindowResize( Uint32 Width, Uint32 Height )
{
    LOG_INFO_MESSAGE( "ComputeParticles::WindowResize| size: [", Width, ", ", Height, "]" );

    // Update projection matrix.
    float aspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
    mCamera.SetProjAttribs( 0.1f, 1000.0f, aspectRatio, PI_F / 4.0f, m_pSwapChain->GetDesc().PreTransform, m_pDevice->GetDeviceInfo().IsGLDevice() );

    // TODO: re-enable once Canvas pixel shader is reworked to be drawing in pixel coordinates
    //if( mBackgroundCanvas ) {
    //    mBackgroundCanvas->setSize( int2( Width, Height ) );
    //}

    // --------------------
    // Post Process

    // Set minimal render target size
    auto blah = 1u << DownSampleFactor;
    Width  = std::max(Width, 1u << DownSampleFactor);
    Height = std::max(Height, 1u << DownSampleFactor);

    // Check if the image needs to be recreated.
	if( m_GBuffer.Color != nullptr && m_GBuffer.Color->GetDesc().Width == Width && m_GBuffer.Color->GetDesc().Height == Height ) {
        return;
    }

    m_GBuffer = {};

	// Create window-size G-buffer textures.
	{
	    TextureDesc RTDesc;
	    RTDesc.Name = "GBuffer Color";
	    RTDesc.Type = RESOURCE_DIM_TEX_2D;
	    RTDesc.Width = Width;
	    RTDesc.Height = Height;
	    RTDesc.MipLevels = DownSampleFactor;
	    RTDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
	    RTDesc.Format = app::global()->colorBufferFormat;
	    m_pDevice->CreateTexture( RTDesc, nullptr, &m_GBuffer.Color );

	    // Create texture view
	    for( Uint32 Mip = 0; Mip < DownSampleFactor; ++Mip ) {
		    TextureViewDesc ViewDesc;
		    ViewDesc.ViewType = TEXTURE_VIEW_RENDER_TARGET;
		    ViewDesc.TextureDim = RESOURCE_DIM_TEX_2D;
		    ViewDesc.MostDetailedMip = Mip;
		    ViewDesc.NumMipLevels = 1;

		    m_GBuffer.Color->CreateView( ViewDesc, &m_GBuffer.ColorRTVs[Mip] );

		    ViewDesc.ViewType = TEXTURE_VIEW_SHADER_RESOURCE;
		    m_GBuffer.Color->CreateView( ViewDesc, &m_GBuffer.ColorSRBs[Mip] );
	    }

	    RTDesc.Name = "GBuffer Depth";
	    RTDesc.MipLevels = 1;
	    RTDesc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
	    RTDesc.Format = app::global()->depthBufferFormat;
	    m_pDevice->CreateTexture( RTDesc, nullptr, &m_GBuffer.Depth );

	    // Create post-processing SRB
		mPostProcessSRB.Release();
        if( mPostProcessPSO ) {
            mPostProcessPSO->CreateShaderResourceBinding( &mPostProcessSRB );
            mPostProcessSRB->GetVariableByName( SHADER_TYPE_PIXEL, "PostProcessConstantsCB" )->Set( mPostProcessConstantsBuffer );
            mPostProcessSRB->GetVariableByName( SHADER_TYPE_PIXEL, "g_GBuffer_Color" )->Set( m_GBuffer.Color->GetDefaultView( TEXTURE_VIEW_SHADER_RESOURCE ) );
            mPostProcessSRB->GetVariableByName( SHADER_TYPE_PIXEL, "g_GBuffer_Depth" )->Set( m_GBuffer.Depth->GetDefaultView( TEXTURE_VIEW_SHADER_RESOURCE ) );
        }
	}

	// Create down sample SRB
	for( Uint32 Mip = 0; Mip < DownSampleFactor; ++Mip ) {
		auto& SRB = mDownSampleSRB[Mip];
		SRB.Release();
		mDownSamplePSO->CreateShaderResourceBinding( &SRB );
		SRB->GetVariableByName( SHADER_TYPE_PIXEL, "g_GBuffer_Color" )->Set( m_GBuffer.ColorSRBs[Mip] );
	}

    // Create window-size offscreen render target to render post-processing into, so we can anti-alias after
    {
        TextureDesc RTColorDesc;
        RTColorDesc.Name      = "Post Process Render Target";
        RTColorDesc.Type      = RESOURCE_DIM_TEX_2D;
        RTColorDesc.Width     = m_pSwapChain->GetDesc().Width;
        RTColorDesc.Height    = m_pSwapChain->GetDesc().Height;
        RTColorDesc.MipLevels = 1;
        RTColorDesc.Format    = m_pSwapChain->GetDesc().ColorBufferFormat;
        RTColorDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;

        RTColorDesc.ClearValue.Format   = RTColorDesc.Format;
        RTColorDesc.ClearValue.Color[0] = 0.350f;
        RTColorDesc.ClearValue.Color[1] = 0.350f;
        RTColorDesc.ClearValue.Color[2] = 0.350f;
        RTColorDesc.ClearValue.Color[3] = 1.f;

        RefCntAutoPtr<ITexture> pRTColor;
        m_pDevice->CreateTexture( RTColorDesc, nullptr, &pRTColor );
        // Store the render target view
        mPostProcessRTV = pRTColor->GetDefaultView( TEXTURE_VIEW_RENDER_TARGET );

        if( mFXAA ) {
           mFXAA->setTexture( pRTColor->GetDefaultView( TEXTURE_VIEW_SHADER_RESOURCE ) );
        }
    }
}

void ComputeParticles::Update( double CurrTime, double ElapsedTime )
{
    SampleBase::Update( CurrTime, ElapsedTime );
    updateUI();

    checkReloadOnAssetsUpdated();

    mTime = (float)CurrTime;
    mTimeDelta = (float)ElapsedTime;
    mCamera.Update( m_InputController, mTimeDelta );

    // Build a transform matrix for the test solid
    float4x4 modelTransform = float4x4::Identity();
    modelTransform *= float4x4::Scale( TestSolidScale );
    //if( TestSolidRotate )  {
    //    modelTransform *= float4x4::RotationY( static_cast<float>(CurrTime) * 1.0f) * float4x4::RotationX( -PI_F * 0.1f );
    //}
    if( TestSolidUseLookAt ) {
        TestSolidLookAt = normalize( TestSolidLookAt );
        auto rot = GetRotationQuat( float3( 0, 1, 0 ), TestSolidLookAt, float3( 0, 1, 0 ) );
        modelTransform *= rot.ToMatrix();
    }
    else {
        modelTransform *= TestSolidRotation.ToMatrix();
    }

    modelTransform *= float4x4::Translation( TestSolidTranslate );

    if( mBackgroundCanvas ) {
        mBackgroundCanvas->update( ElapsedTime );
    }

    if( mTestSolid ) {
        mTestSolid->setLightDir( LightDir );
        mTestSolid->update( ElapsedTime );
        mTestSolid->setTransform( modelTransform );
    }
    if( mParticleSolid ) {
        mParticleSolid->setLightDir( LightDir );
        mParticleSolid->update( ElapsedTime );
        //mParticleSolid->setTransform( modelTransform );
    }

    if( UseFirstPersonCamera ) {
        mViewProjMatrix = mCamera.GetViewMatrix() * mCamera.GetProjMatrix();
        //mWorldViewProjMatrix = modelTransform * mCamera.GetViewMatrix() * mCamera.GetProjMatrix();
    }
    else {
        // from samples..
        // 
        // Camera is at (0, 0, -5) looking along the Z axis
        float4x4 View = float4x4::Translation(0.f, 0.0f, 5.0f);

        // Get pretransform matrix that rotates the scene according the surface orientation
        // TODO: remove to simplify for now, this is the identity matrix on desktop
        auto SrfPreTransform = GetSurfacePretransformMatrix(float3{0, 0, 1});

        // Get projection matrix adjusted to the current screen orientation
        //auto Proj = GetAdjustedProjectionMatrix(PI_F / 4.0f, 0.1f, 100.f);
        auto Proj = GetAdjustedProjectionMatrix( mCamera.GetProjAttribs().FOV, mCamera.GetProjAttribs().NearClipPlane, mCamera.GetProjAttribs().FarClipPlane );

        mViewProjMatrix = View * SrfPreTransform * Proj;
        //mWorldViewProjMatrix = modelTransform * View * SrfPreTransform * Proj;
    }
}

// Render a frame
void ComputeParticles::Render()
{
    const float gray = 0.00f;
    const float ClearColor[] = { gray, gray, gray, 0.0f}; // alpha channel is for glow intensity

    //auto* rtv = m_pSwapChain->GetCurrentBackBufferRTV();
    //auto* dsv = m_pSwapChain->GetDepthBufferDSV();
    
    // Render to GBuffer
	ITextureView* rtv = m_GBuffer.Color->GetDefaultView( TEXTURE_VIEW_RENDER_TARGET );
	ITextureView* dsv = m_GBuffer.Depth->GetDefaultView( TEXTURE_VIEW_DEPTH_STENCIL );
	m_pImmediateContext->SetRenderTargets( 1, &rtv, dsv, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

    // TODO: tut23 says "transitions is not needed" - why not?
    // - uses RESOURCE_STATE_TRANSITION_MODE_VERIFY instead
    m_pImmediateContext->ClearRenderTarget( rtv, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
    m_pImmediateContext->ClearDepthStencil( dsv, CLEAR_DEPTH_FLAG, 1.0f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

    if( mBackgroundCanvas && mDrawBackground ) {
        float4x4 cameraViewProj = mCamera.GetViewMatrix() * mCamera.GetProjMatrix();
        auto pixelConstants = mBackgroundCanvas->getPixelConstantsBuffer();
        MapHelper<BackgroundPixelConstants> cb( m_pImmediateContext, pixelConstants, MAP_WRITE, MAP_FLAG_DISCARD );
        cb->inverseViewProj = cameraViewProj.Inverse().Transpose();
        cb->camPos = mCamera.GetPos();
        cb->camDir = mCamera.GetWorldAhead();

        auto swapChainDesc = m_pSwapChain->GetDesc();
        cb->resolution = float2( swapChainDesc.Width, swapChainDesc.Height );

        mBackgroundCanvas->render( m_pImmediateContext, mViewProjMatrix );
    }

    // update ParticleConstants cbuffer
    // appears we always need to update this buffer or an assert failure happens (stale buffer)
    {
        MapHelper<ParticleConstants> constData( m_pImmediateContext, mParticleConstants, MAP_WRITE, MAP_FLAG_DISCARD );
        constData->viewProj = mViewProjMatrix.Transpose();
        constData->numParticles = static_cast<Uint32>( mNumParticles );
        constData->deltaTime     = std::min( mTimeDelta, 1.f / 60.f) * mSimulationSpeed;
        constData->time = mTime;
        constData->scale = mParticleScale;
        constData->gridSize = mGridSize;
        constData->speedMinMax = mSpeedMinMax;
        constData->separation = mSeparation;
        constData->alignment = mAlignment;
        constData->cohesion = mCohesion;
        constData->separationDist = mSeparationDist;
        constData->alignmentDist = mAlignmentDist;
        constData->cohesionDist = mCohesionDist;
    }

    updateParticles();
    drawParticles();

    if( mTestSolid && mDrawTestSolid ) {
        mTestSolid->draw( m_pImmediateContext, mViewProjMatrix );
    }

    if( mPostProcessConstants.glowEnabled ) {
        DownSample();
    }

    // Final pass
    // TODO: cleanup
    ITextureView *mainRenderTarget = m_pSwapChain->GetCurrentBackBufferRTV();
    if( ! mFXAAEnabled ) {
        m_pImmediateContext->SetRenderTargets( 1, &mainRenderTarget, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

        PostProcess();
    }


    if( mFXAAEnabled ) {
        // TODO: render to a new target and then pass its TextureView into FXAA apply
        // - code below needs to be called from FXAA->preDraw() or something
        // - can depth buffer view be null?
        
        // Clear the offscreen render target and depth buffer
        const float clearColor[] = { 0.0f, 0.05f, 0.4f, 1.0f };
        m_pImmediateContext->SetRenderTargets( 1, &mPostProcessRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
        m_pImmediateContext->ClearRenderTarget( mPostProcessRTV, clearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
        //m_pImmediateContext->ClearDepthStencil(m_pDepthDSV, CLEAR_DEPTH_FLAG, 1.0f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        PostProcess();

        // bind the main render target and render AA into main target
        m_pImmediateContext->SetRenderTargets( 1, &mainRenderTarget, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

        JU_PROFILE( "FXAA", m_pImmediateContext, mProfiler.get() );
        mFXAA->apply( m_pImmediateContext, m_GBuffer.Color->GetDefaultView( TEXTURE_VIEW_SHADER_RESOURCE ) );
    }
}

void ComputeParticles::updateParticles()
{
    if( ! mResetParticleListsPSO || ! mMoveParticlesPSO || ! mInteractParticlesPSO ) {
        return;
    }

    if( mUpdateParticles ) {
        DispatchComputeAttribs dispatchAttribs;
        dispatchAttribs.ThreadGroupCountX = ( mNumParticles + mThreadGroupSize - 1) / mThreadGroupSize;

        {
            JU_PROFILE( "reset particles", m_pImmediateContext, mProfiler.get() );
            m_pImmediateContext->SetPipelineState( mResetParticleListsPSO );
            m_pImmediateContext->CommitShaderResources( mResetParticleListsSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
            m_pImmediateContext->DispatchCompute( dispatchAttribs );
        }

        {
            JU_PROFILE( "move particles", m_pImmediateContext, mProfiler.get() );
            m_pImmediateContext->SetPipelineState( mMoveParticlesPSO );
            m_pImmediateContext->CommitShaderResources( mMoveParticlesSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
            m_pImmediateContext->DispatchCompute( dispatchAttribs );
        }

        {
            JU_PROFILE( "interact particles", m_pImmediateContext, mProfiler.get() );
            m_pImmediateContext->SetPipelineState( mInteractParticlesPSO );
            m_pImmediateContext->CommitShaderResources( mInteractParticlesSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
            m_pImmediateContext->DispatchCompute( dispatchAttribs );
        }
    }

#if DEBUG_PARTICLE_BUFFERS
    if( mDebugCopyParticles ) {
        m_pImmediateContext->CopyBuffer( mParticleAttribsBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
            mParticleAttribsStaging, 0, mNumParticles * sizeof(ParticleAttribs), RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
        m_pImmediateContext->CopyBuffer( mParticleListsBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
            mParticleListsStaging, 0, mNumParticles * sizeof(int), RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
        m_pImmediateContext->CopyBuffer( mParticleListHeadsBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
            mParticleListsHeadStaging, 0, mNumParticles * sizeof(int), RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

        // We should use synchronizations to safely access the mapped memory.
        // TODO: fix and re-enable this, but not crucial when looking at debug data
        //m_pImmediateContext->EnqueueSignal( mFenceParticleAttribsAvailable, mFenceParticleAttribsValue++ );
        //m_pImmediateContext->Flush(); // TODO: needed?

        //// TODO: check if this causes the main loop to wait
        //m_pImmediateContext->DeviceWaitForFence( mFenceParticleAttribsAvailable, mFenceParticleAttribsValue );

        // TODO: copy with std::copy or memcopy
        DebugParticleAttribsData.resize( mNumParticles );
        {
            MapHelper<ParticleAttribs> stagingData( m_pImmediateContext, mParticleAttribsStaging, MAP_READ, MAP_FLAG_DO_NOT_WAIT );
            if( stagingData ) {
                for( size_t i = 0; i < mNumParticles; i++ ) {
                    const ParticleAttribs &p = stagingData[i];
                    DebugParticleAttribsData.at( i ) = p; 
                }
            }
        }
        DebugParticleListsData.resize( mNumParticles );
        {
            MapHelper<int> stagingData( m_pImmediateContext, mParticleListsStaging, MAP_READ, MAP_FLAG_DO_NOT_WAIT );
            if( stagingData ) {
                for( size_t i = 0; i < mNumParticles; i++ ) {
                    const int &p = stagingData[i];
                    DebugParticleListsData.at( i ) = p; 
                }
            }
        }
        DebugParticleListsHeadData.resize( mNumParticles );
        {
            MapHelper<int> stagingData( m_pImmediateContext, mParticleListsHeadStaging, MAP_READ, MAP_FLAG_DO_NOT_WAIT );
            if( stagingData ) {
                for( size_t i = 0; i < mNumParticles; i++ ) {
                    const int &p = stagingData[i];
                    DebugParticleListsHeadData.at( i ) = p; 
                }
            }
        }
    }
#endif

    //mProfiler->end( m_pImmediateContext, "update particles" );
}

void ComputeParticles::drawParticles()
{
    if( ! mDrawParticles || ! mRenderParticlePSO ) {
        return;
    }

    JU_PROFILE( "draw particles", m_pImmediateContext, mProfiler.get() );

    m_pImmediateContext->SetPipelineState( mRenderParticlePSO );
    m_pImmediateContext->CommitShaderResources( mRenderParticleSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

    if( mParticleType == ParticleType::Sprite ) {
        DrawAttribs drawAttrs;
        drawAttrs.NumVertices  = 4;
        drawAttrs.NumInstances = static_cast<Uint32>( mNumParticles );
        m_pImmediateContext->Draw(drawAttrs);
    }
    else {
        mParticleSolid->draw( m_pImmediateContext, mViewProjMatrix, mNumParticles );
    }
}

// ------------------------------------------------------------------------------------------------------------
// Post Process
// ------------------------------------------------------------------------------------------------------------
// TODO: move to separate class

void ComputeParticles::initPostProcessPSO()
{
    //ShaderMacroHelper Macros;
    //Macros.AddShaderMacro("GLOW", 1);

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name         = "Post process PSO";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    PSOCreateInfo.GraphicsPipeline.NumRenderTargets                  = 1;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                     = m_pSwapChain->GetDesc().ColorBufferFormat;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology                 = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable      = false;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = false;

    const SamplerDesc SamLinearClampDesc {
        FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR,
        TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP
    };
    const ImmutableSamplerDesc ImtblSamplers[] = {
        {SHADER_TYPE_PIXEL, "g_GBuffer_Color", SamLinearClampDesc}
    };
    PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType  = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

    ShaderCreateInfo shaderCI;
    shaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    shaderCI.pShaderSourceStreamFactory = app::global()->shaderSourceFactory;

	RefCntAutoPtr<IShader> vertShader;
	{
		shaderCI.Desc = { "Post process VS", SHADER_TYPE_VERTEX, true };
		shaderCI.EntryPoint = "main";
		shaderCI.FilePath = "shaders/post/post_process.vsh";
		m_pDevice->CreateShader( shaderCI, &vertShader );
	}

	RefCntAutoPtr<IShader> pixelShader;
	{
		shaderCI.Desc = { "Post process PS", SHADER_TYPE_PIXEL, true };
		shaderCI.EntryPoint = "main";
        shaderCI.FilePath = "shaders/post/post_process.psh";
		m_pDevice->CreateShader( shaderCI, &pixelShader );
	}

    PSOCreateInfo.pVS = vertShader;
    PSOCreateInfo.pPS = pixelShader;

    mPostProcessPSO.Release();
    m_pDevice->CreateGraphicsPipelineState( PSOCreateInfo, &mPostProcessPSO );

    // downsample buffers for glow
    RefCntAutoPtr<IShader> downSamplePS;
    {
        shaderCI.Desc       = { "Down sample PS", SHADER_TYPE_PIXEL, true };
        shaderCI.EntryPoint = "main";
        shaderCI.FilePath   = "shaders/post/downsample.psh";
        m_pDevice->CreateShader( shaderCI, &downSamplePS );
    }
    PSOCreateInfo.pPS = downSamplePS;

    PSOCreateInfo.PSODesc.Name                   = "Downsample PSO";
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0] = app::global()->colorBufferFormat;

    PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers    = nullptr;
    PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = 0;

    mDownSamplePSO.Release();
    m_pDevice->CreateGraphicsPipelineState( PSOCreateInfo, &mDownSamplePSO );
}

void ComputeParticles::DownSample()
{
    JU_PROFILE( "downsample", m_pImmediateContext, mProfiler.get() );

	m_pImmediateContext->SetPipelineState( mDownSamplePSO );
	m_pImmediateContext->SetVertexBuffers( 0, 0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE, SET_VERTEX_BUFFERS_FLAG_RESET );
	m_pImmediateContext->SetIndexBuffer( nullptr, 0, RESOURCE_STATE_TRANSITION_MODE_NONE );

	StateTransitionDesc Barrier{ m_GBuffer.Color, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE, 0u, 1u };

	for( Uint32 Mip = 1; Mip < DownSampleFactor; ++Mip ) {
		Barrier.FirstMipLevel = Mip - 1;
		m_pImmediateContext->TransitionResourceStates( 1, &Barrier );

		m_pImmediateContext->SetRenderTargets( 1, &m_GBuffer.ColorRTVs[Mip], nullptr, RESOURCE_STATE_TRANSITION_MODE_VERIFY );

		m_pImmediateContext->CommitShaderResources( mDownSampleSRB[Mip - 1], RESOURCE_STATE_TRANSITION_MODE_NONE );
		m_pImmediateContext->Draw( DrawAttribs{ 3, DRAW_FLAG_VERIFY_DRAW_ATTRIBS | DRAW_FLAG_VERIFY_STATES } );
	}

	// Transit last mipmap level to SRV state.
	// Now all mipmaps in m_GBuffer.Color are in SRV state, so update resource state.
	Barrier.FirstMipLevel = DownSampleFactor - 1;
	Barrier.Flags = STATE_TRANSITION_FLAG_UPDATE_STATE;
	m_pImmediateContext->TransitionResourceStates( 1, &Barrier );
}

void ComputeParticles::PostProcess()
{
    JU_PROFILE( "post process", m_pImmediateContext, mProfiler.get() );

    const auto ViewProj    = mCamera.GetViewMatrix() * mCamera.GetProjMatrix();
    const auto ViewProjInv = ViewProj.Inverse();

    mPostProcessConstants.viewProjInv = ViewProjInv.Transpose();
    mPostProcessConstants.cameraPos   = mCamera.GetPos();

	m_pImmediateContext->UpdateBuffer( mPostProcessConstantsBuffer, 0, sizeof( mPostProcessConstants ), &mPostProcessConstants, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

	m_pImmediateContext->SetPipelineState( mPostProcessPSO );
	m_pImmediateContext->CommitShaderResources( mPostProcessSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

	m_pImmediateContext->SetVertexBuffers( 0, 0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE, SET_VERTEX_BUFFERS_FLAG_RESET );
	m_pImmediateContext->SetIndexBuffer( nullptr, 0, RESOURCE_STATE_TRANSITION_MODE_NONE );

	m_pImmediateContext->Draw( DrawAttribs{ 3, DRAW_FLAG_VERIFY_ALL } );
}

// ------------------------------------------------------------------------------------------------------------
// ImGui
// ------------------------------------------------------------------------------------------------------------

void ComputeParticles::updateUI()
{
    if( ! mUIEnabled )
        return;

    im::SetNextWindowPos( ImVec2( 10, 10 ), ImGuiCond_FirstUseEver );
    if( im::Begin( "Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) ) {
        im::Checkbox( "ui", &mUIEnabled );
        im::Checkbox( "profiling ui", &mProfilingUIEnabled );
        if( im::CollapsingHeader( "Particles", ImGuiTreeNodeFlags_DefaultOpen ) ) {
            im::Checkbox( "update", &mUpdateParticles );
            im::SameLine();
            im::Checkbox( "draw", &mDrawParticles );

            if( im::InputInt( "count", &mNumParticles, 100, 1000, ImGuiInputTextFlags_EnterReturnsTrue ) ) {
                mNumParticles = std::min( std::max( mNumParticles, 10 ), 10000000 ); // max: 10million
                initParticleBuffers();
                initSolids(); // TODO: see comment for 'init particle buffers' button
            }
            im::SliderFloat( "speed", &mSimulationSpeed, 0.1f, 5.f );
            im::DragFloat( "scale", &mParticleScale, 0.01f, 0.001f, 100.0f );

            im::Text( "grid size: [%d, %0d, %d]", mGridSize.x, mGridSize.y, mGridSize.y );

            static std::vector<const char*> types = { "sprite", "cube", "pyramid" };
            int t = (int)mParticleType;
            if( im::Combo( "type", &t, types.data(), (int)types.size() ) ) {
                LOG_INFO_MESSAGE( "solid type changed" );
                mParticleType = (ParticleType)t;

                initSolids();
                initRenderParticlePSO();
            }
            if( im::Button( "init particle buffers" ) ) {
                initParticleBuffers();
                initSolids(); // TODO: this is needed to update the ParticleAttribs var, try just setting that on mParticleSolid
            }
#if DEBUG_PARTICLE_BUFFERS
            im::Checkbox( "debug copy particles", &mDebugCopyParticles );
            if( mDebugCopyParticles ) {
                im::Indent();
                im::Checkbox( "ParticleAttribs ui", &DebugShowParticleAttribsWindow );
                im::Checkbox( "ParticleLists ui", &DebugShowParticleListssWindow );
                im::Unindent();
            }
#endif
            im::Separator();
            im::Text( "Flocking" );            
            ImGui::DragFloatRange2("speed", &mSpeedMinMax.x, &mSpeedMinMax.y, 0.02f, 0.0f, 100.0f, "min: %6.3f", "max: %6.3f", ImGuiSliderFlags_AlwaysClamp);
            im::DragFloat( "separation", &mSeparation, 0.001f, 0.0002f, 2.0f );
            im::DragFloat( "alignment", &mAlignment, 0.001f, 0.0002f, 2.0f );
            im::DragFloat( "cohesion", &mCohesion, 0.001f, 0.0002f, 2.0f );

            // TODO: make sure dists for separation < align < cohesion
            im::DragFloat( "separation dist", &mSeparationDist, 0.001f, 0.0f, 2.0f );
            im::DragFloat( "alignment dist", &mAlignmentDist, 0.001f, 0.0f, 2.0f );
            im::DragFloat( "cohesion dist", &mCohesionDist, 0.001f, 0.0f, 2.0f );
        }

        if( im::CollapsingHeader( "Camera", ImGuiTreeNodeFlags_DefaultOpen ) ) {
            im::Checkbox( "enabled", &UseFirstPersonCamera );
            if( im::DragFloat( "move speed", &CameraMoveSpeed, 0.01f, 0.001f, 100.0f ) ) {
                mCamera.SetMoveSpeed( CameraMoveSpeed );
            }
            if( im::DragFloat( "rotate speed", &CameraRotationSpeed ) ) {
                mCamera.SetRotationSpeed( CameraRotationSpeed );
            }
            if( im::DragFloat2( "speed up scale", &CameraSpeedUp.x ) ) {
                mCamera.SetSpeedUpScales( CameraSpeedUp.x, CameraSpeedUp.y );
            }
            if( im::Button("reset") ) {
                initCamera();
            }
            // TODO: draw this with 3 axes (may need a quaternion, need to look at the author's examples)
            float3 camPos = mCamera.GetPos();
            if( im::DragFloat3( "pos##cam", &camPos.x, 0.01f ) ) {
                mCamera.SetPos( camPos );
            }
            im::Text( "view dir: [%0.02f, %0.02f, %0.02f]", mCamera.GetWorldAhead().x, mCamera.GetWorldAhead().y, mCamera.GetWorldAhead().z );
            auto viewDir = mCamera.GetWorldAhead();
            im::gizmo3D( "##ViewDir", viewDir, ImGui::GetTextLineHeight() * 5 );
        }

        if( im::CollapsingHeader( "Scene", ImGuiTreeNodeFlags_DefaultOpen ) ) {
            im::Text( "light dir: [%0.02f, %0.02f, %0.02f]", LightDir.x, LightDir.y, LightDir.z );
            im::gizmo3D( "##LightDirection", LightDir, ImGui::GetTextLineHeight() * 5 );

            im::Separator();
            im::Text( "Test Solid" );
            im::Checkbox( "draw##test solid", &mDrawTestSolid );
            im::SameLine();
            im::Checkbox( "use lookat##solid", &TestSolidUseLookAt );
            if( TestSolidUseLookAt ) {
                im::DragFloat3( "lookat dir##solid", &TestSolidLookAt.x, 0.01f );
                im::gizmo3D( "##TestSolidLookAt", TestSolidLookAt, ImGui::GetTextLineHeight() * 7 );
            }
            else {
                im::gizmo3D( "##TestSolidRotation", TestSolidRotation, ImGui::GetTextLineHeight() * 7 );
            }
            im::DragFloat3( "translate##solid", &TestSolidTranslate.x, 0.01f );

            static bool lockDims = false;
            if( lockDims ) {
                if( im::DragFloat( "scale##solids1", &TestSolidScale.x, 0.01f, 0.001f, 1000.0f ) ) {
                    TestSolidScale = float3( TestSolidScale.x, TestSolidScale.x, TestSolidScale.x );
                }
            }
            else {
                im::DragFloat3( "scale##solid", &TestSolidScale.x, 0.01f );
            }
            im::Checkbox( "lock dims##test solid", &lockDims );


            im::Separator();
            im::Text( "Background Canvas" );
            im::Checkbox( "draw##background", &mDrawBackground );
            auto bgCenter = mBackgroundCanvas->getCenter();
            if( im::DragFloat2( "bg center", &bgCenter.x, 0.02f ) ) {
                mBackgroundCanvas->setCenter( bgCenter );
            }
            auto bgSize = mBackgroundCanvas->getSize();
            if( im::DragFloat2( "bg size", &bgSize.x, 0.02f ) ) {
                mBackgroundCanvas->setSize( bgSize );
            }
        }

        if( im::CollapsingHeader( "Post Process", ImGuiTreeNodeFlags_DefaultOpen ) ) {
            bool glowEnabled = mPostProcessConstants.glowEnabled;
            if( im::Checkbox( "glow", &glowEnabled ) ) {
                mPostProcessConstants.glowEnabled = int(glowEnabled);
            }
            im::DragFloat( "glow intensity", &mPostProcessConstants.glowIntensity, 0.002f, 0.0001f, 10.0f );

            bool fogEnabled = mPostProcessConstants.fogEnabled;
            if( im::Checkbox( "fog", &fogEnabled ) ) {
                mPostProcessConstants.fogEnabled = int(fogEnabled);
            }
            im::ColorEdit3( "fog color", &mPostProcessConstants.fogColor.r, ImGuiColorEditFlags_Float );
            im::DragFloat( "fog intensity", &mPostProcessConstants.fogIntensity, 0.002f, 0.0001f, 2.0f );

            im::Separator();
            im::Text( "Antialiasing" );
            im::Checkbox( "FXAA", &mFXAAEnabled );
            if( mFXAA ) {
                mFXAA->updateUI();
            }

            im::Separator();
            //im::Text( "GBuffer.Color" );
            //im::Image( m_GBuffer.Color->GetDefaultView( TEXTURE_VIEW_SHADER_RESOURCE ), { 300, 200 } );

            static int colorSRBMip = 0;
            im::Text( "GBuffer.ColorSRBs[]" );
            im::DragInt( "mip", &colorSRBMip, 0.1f, 0, DownSampleFactor - 1 );
            im::Image( m_GBuffer.ColorSRBs[colorSRBMip], { 300, 200 } );
        }
    }
    im::End(); // Settings

#if DEBUG_PARTICLE_BUFFERS
    if( mDebugCopyParticles ) {
        updateDebugParticleDataUI();
    }

#endif

    if( mProfilingUIEnabled && mProfiler ) {
        mProfiler->updateUI( &mProfilingUIEnabled );
    }
}

#if DEBUG_PARTICLE_BUFFERS
void ComputeParticles::updateDebugParticleDataUI()
{
    im::SetNextWindowPos( { 400, 20 }, ImGuiCond_FirstUseEver );
    im::SetNextWindowSize( { 800, 700 }, ImGuiCond_FirstUseEver );

    if( DebugShowParticleAttribsWindow && im::Begin( "ParticleAttribs", &DebugShowParticleAttribsWindow ) ) {
        im::Text( "count: %d", mNumParticles );

        if( ! DebugParticleAttribsData.empty() ) {
            static int maxRows = 1000;

            static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Hideable;
            flags |= ImGuiTableFlags_ScrollY;
            flags |= ImGuiTableFlags_SizingFixedFit;

            if( im::BeginTable( "table_ParticleAttribs", 6, flags ) ) {
                ImGuiTableColumnFlags columnFlags = ImGuiTableColumnFlags_WidthFixed; 
                im::TableSetupScrollFreeze( 0, 1 ); // Make top row always visible
                im::TableSetupColumn( "index", columnFlags, 34 );
                im::TableSetupColumn( "pos", columnFlags, 180 );
                im::TableSetupColumn( "vel", columnFlags, 180 );
                im::TableSetupColumn( "accel", columnFlags, 180 );
                im::TableSetupColumn( "interactions", columnFlags, 50 );
                im::TableSetupColumn( "temp", ImGuiTableColumnFlags_None );
                im::TableHeadersRow();

                ImGuiListClipper clipper;
                clipper.Begin( std::min( maxRows, mNumParticles ) );
                while( clipper.Step() ) {
                    for( int row = clipper.DisplayStart; row<clipper.DisplayEnd; row++ ) {
                        im::TableNextRow();
                        const auto &p = DebugParticleAttribsData.at( row );
                        int column = 0;
                        im::TableSetColumnIndex( column++ );
                        im::Text( "%d", row );
                        im::TableSetColumnIndex( column++ );
                        im::Text( "[%6.3f, %6.3f, %6.3f]", p.pos.x, p.pos.y, p.pos.z );
                        im::TableSetColumnIndex( column++ );
                        im::Text( "[%6.3f, %6.3f, %6.3f]", p.vel.x, p.vel.y, p.vel.z );
                        im::TableSetColumnIndex( column++ );
                        im::Text( "[%6.3f, %6.3f, %6.3f]", p.accel.x, p.accel.y, p.accel.z );
                        im::TableSetColumnIndex( column++ );
                        im::Text( " %d", p.numInteractions );
                        im::TableSetColumnIndex( column++ );
                        im::Text( "%0.03f", p.temperature );
                    }
                }
                im::EndTable();
            }
        }

        im::End(); // ParticleAttribs
    }

    im::SetNextWindowPos( { 500, 40 }, ImGuiCond_FirstUseEver );
    im::SetNextWindowSize( { 600, 600 }, ImGuiCond_FirstUseEver );

    if( DebugShowParticleListssWindow && im::Begin( "ParticleLists", &DebugShowParticleListssWindow ) ) {
        // TODO: show either as a table or tree

        if( ! DebugParticleListsData.empty() && ! DebugParticleListsHeadData.empty() ) {
            static int maxRows = 1000;

            static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Hideable;
            flags |= ImGuiTableFlags_ScrollY;
            flags |= ImGuiTableFlags_SizingFixedFit;

            if( im::BeginTable( "table_ParticleAttribs", 3, flags ) ) {
                ImGuiTableColumnFlags columnFlags = ImGuiTableColumnFlags_WidthFixed; 
                im::TableSetupScrollFreeze( 0, 1 ); // Make top row always visible
                im::TableSetupColumn( "index", columnFlags, 40 );
                im::TableSetupColumn( "head", columnFlags, 40 );
                im::TableSetupColumn( "next", columnFlags, 40 );
                im::TableHeadersRow();

                ImGuiListClipper clipper;
                clipper.Begin( std::min( maxRows, mNumParticles ) );
                while( clipper.Step() ) {
                    for( int row = clipper.DisplayStart; row<clipper.DisplayEnd; row++ ) {
                        im::TableNextRow();
                        int headIndex = DebugParticleListsHeadData.at( row );
                        int nextIndex = DebugParticleListsData.at( row );
                        int column = 0;
                        im::TableSetColumnIndex( column++ );
                        im::Text( "%d", row );
                        im::TableSetColumnIndex( column++ );
                        im::Text( "%d", headIndex );
                        im::TableSetColumnIndex( column++ );
                        im::Text( "%d", nextIndex );
                    }
                }
                im::EndTable();
            }
        }

        im::End(); // ParticleAttribs
    }

}
#endif