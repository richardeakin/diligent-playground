
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
    float  padding0; // TODO: try without padding between the positions
    float3 newPos;
    float  padding1;

    float3 speed;
    float  padding2;
    float3 newSpeed;
    float  padding3;

    float size          = 0;
    float temperature   = 0;
    int   numCollisions = 0;
    float padding4      = 0;
};

struct ParticleConstants {
    float4x4 viewProj;

    uint  numParticles;
    float deltaTime;
    float padding0;
    float padding1;

    float scale;
    int3  gridSize;
};

struct BackgroundPixelConstants {
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

ju::FileWatchHandle     ShadersDirWatchHandle;
bool                    ShaderAssetsMarkedDirty = false;

std::vector<ParticleAttribs> DebugParticleData;

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
}

void ComputeParticles::Initialize( const SampleInitInfo& InitInfo )
{
    ju::initLivePP( LPP_PATH );

    SampleBase::Initialize( InitInfo );

    initConsantBuffer();
    initRenderParticlePSO();
    initUpdateParticlePSO();
    initParticleBuffers();

    auto global = app::global();
    global->renderDevice = m_pDevice;
    global->swapChainImageDesc = &m_pSwapChain->GetDesc();
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory( nullptr, &global->shaderSourceFactory );

    mBackgroundCanvas = std::make_unique<ju::Canvas>( sizeof(BackgroundPixelConstants) );
    initSolids();
    initCamera();

    mProfiler = std::make_unique<ju::Profiler>( m_pDevice );

    watchShadersDir();
}

void ComputeParticles::initRenderParticlePSO()
{
    mRenderParticlePSO.Release();

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name = "Render particles PSO";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    // This tutorial will render to a single render target
    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = m_pSwapChain->GetDesc().ColorBufferFormat;
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = m_pSwapChain->GetDesc().DepthBufferFormat;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = false;

    auto& BlendDesc = PSOCreateInfo.GraphicsPipeline.BlendDesc;
    BlendDesc.RenderTargets[0].BlendEnable = True;
    BlendDesc.RenderTargets[0].SrcBlend    = BLEND_FACTOR_SRC_ALPHA;
    BlendDesc.RenderTargets[0].DestBlend   = BLEND_FACTOR_INV_SRC_ALPHA;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.Desc.UseCombinedTextureSamplers = true; // needed for OpenGL support
    // TODO: enable these lines and see if any of my hlsl compile errors are fixed
    //ShaderCI.ShaderCompiler = SHADER_COMPILER_DXC; // use modern HLSL compiler
    //ShaderCI.HLSLVersion    = {6, 3};

    // Create a shader source stream factory to load shaders from files.
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory( nullptr, &pShaderSourceFactory );
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
    // Create particle vertex shader
    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Particle VS";
        ShaderCI.FilePath        = "shaders/particles/particle_sprite.vsh";
        m_pDevice->CreateShader( ShaderCI, &pVS );
    }

    // Create particle pixel shader
    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Particle PS";
        ShaderCI.FilePath        = "shaders/particles/particle_sprite.psh";

        m_pDevice->CreateShader( ShaderCI, &pPS );
    }

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    // Define variable type that will be used by default
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    // Shader variables should typically be mutable, which means they are expected to change on a per-instance basis
    ShaderResourceVariableDesc Vars[] = {
        { SHADER_TYPE_VERTEX, "Particles", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE }
    };

    PSOCreateInfo.PSODesc.ResourceLayout.Variables    = Vars;
    PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Vars);

    m_pDevice->CreateGraphicsPipelineState( PSOCreateInfo, &mRenderParticlePSO );
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
    mCollideParticlesPSO.Release();
    mUpdateParticleSpeedPSO.Release();

    ShaderCreateInfo ShaderCI;
    // Tell the system that the shader source code is in HLSL.
    // For OpenGL, the engine will convert this into GLSL under the hood.
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;

    // OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
    ShaderCI.Desc.UseCombinedTextureSamplers = true;

    // Create a shader source stream factory to load shaders from files.
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory( nullptr, &pShaderSourceFactory );
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;

    ShaderMacroHelper Macros;
    Macros.AddShaderMacro( "THREAD_GROUP_SIZE", mThreadGroupSize );
    Macros.Finalize();

    RefCntAutoPtr<IShader> pResetParticleListsCS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_COMPUTE;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Reset particle lists CS";
        ShaderCI.FilePath        = "shaders/particles/reset_particle_lists.csh";
        ShaderCI.Macros          = Macros;
        m_pDevice->CreateShader( ShaderCI, &pResetParticleListsCS );
    }

    RefCntAutoPtr<IShader> pMoveParticlesCS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_COMPUTE;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Move particles CS";
        ShaderCI.FilePath        = "shaders/particles/move_particles.csh";
        ShaderCI.Macros          = Macros;
        m_pDevice->CreateShader( ShaderCI, &pMoveParticlesCS );
    }

    RefCntAutoPtr<IShader> pCollideParticlesCS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_COMPUTE;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Collide particles CS";
        ShaderCI.FilePath        = "shaders/particles/collide_particles.csh";
        ShaderCI.Macros          = Macros;
        m_pDevice->CreateShader( ShaderCI, &pCollideParticlesCS );
    }

    RefCntAutoPtr<IShader> pUpdatedSpeedCS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_COMPUTE;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Update particle speed CS";
        ShaderCI.FilePath        = "shaders/particles/collide_particles.csh";
        Macros.AddShaderMacro( "UPDATE_SPEED", 1 );
        ShaderCI.Macros = Macros;
        m_pDevice->CreateShader( ShaderCI, &pUpdatedSpeedCS );
    }

    ComputePipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc&             PSODesc = PSOCreateInfo.PSODesc;

    // init compute pipeline
    PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;
    PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
    ShaderResourceVariableDesc Vars[] = {
        { SHADER_TYPE_COMPUTE, "Constants", SHADER_RESOURCE_VARIABLE_TYPE_STATIC }
    };
    PSODesc.ResourceLayout.Variables    = Vars;
    PSODesc.ResourceLayout.NumVariables = _countof(Vars);

    PSODesc.Name      = "Reset particle lists PSO";
    PSOCreateInfo.pCS = pResetParticleListsCS;
    m_pDevice->CreateComputePipelineState( PSOCreateInfo, &mResetParticleListsPSO );
    if( auto var = mResetParticleListsPSO->GetStaticVariableByName( SHADER_TYPE_COMPUTE, "Constants" ) ) {
        var->Set( mParticleConstants );
    }

    PSODesc.Name      = "Move particles PSO";
    PSOCreateInfo.pCS = pMoveParticlesCS;
    m_pDevice->CreateComputePipelineState( PSOCreateInfo, &mMoveParticlesPSO );
    if( mMoveParticlesPSO ) {
        if( auto var = mMoveParticlesPSO->GetStaticVariableByName( SHADER_TYPE_COMPUTE, "Constants" ) ) {
            var->Set( mParticleConstants );
        }
    }

    PSODesc.Name      = "Collidse particles PSO";
    PSOCreateInfo.pCS = pCollideParticlesCS;
    m_pDevice->CreateComputePipelineState( PSOCreateInfo, &mCollideParticlesPSO );
    if( mCollideParticlesPSO ) {
        mCollideParticlesPSO->GetStaticVariableByName( SHADER_TYPE_COMPUTE, "Constants" )->Set( mParticleConstants );
    }

    PSODesc.Name      = "Update particle speed PSO";
    PSOCreateInfo.pCS = pUpdatedSpeedCS;
    m_pDevice->CreateComputePipelineState( PSOCreateInfo, &mUpdateParticleSpeedPSO );
    if( mUpdateParticleSpeedPSO ) {
        mUpdateParticleSpeedPSO->GetStaticVariableByName( SHADER_TYPE_COMPUTE, "Constants" )->Set( mParticleConstants );
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

    constexpr float fMaxParticleSize = 0.05f;
    float           fSize            = 0.7f / std::sqrt( static_cast<float>( mNumParticles ) );
    fSize                            = std::min( fMaxParticleSize, fSize );
    for ( auto &particle : ParticleData ) {
        particle.newPos.x   = pos_distr(gen);
        particle.newPos.y   = pos_distr(gen);
        particle.newPos.z   = pos_distr(gen);
        particle.newSpeed.x = pos_distr(gen) * fSize * 5.f;
        particle.newSpeed.y = pos_distr(gen) * fSize * 5.f;
        particle.newSpeed.z = pos_distr(gen) * fSize * 5.f;
        particle.size       = fSize * size_distr(gen);
    }

    BufferData VBData;
    VBData.pData    = ParticleData.data();
    VBData.DataSize = sizeof(ParticleAttribs) * static_cast<Uint32>( ParticleData.size() );
    m_pDevice->CreateBuffer( BuffDesc, &VBData, &mParticleAttribsBuffer );
    IBufferView* pParticleAttribsBufferSRV = mParticleAttribsBuffer->GetDefaultView( BUFFER_VIEW_SHADER_RESOURCE );
    IBufferView* pParticleAttribsBufferUAV = mParticleAttribsBuffer->GetDefaultView( BUFFER_VIEW_UNORDERED_ACCESS );

#if DEBUG_PARTICLE_BUFFERS
    // make a staging buffer to read back
    {
        BufferDesc BuffDescStaging;

        BuffDescStaging.Name           = "ParticleAttribs staging buffer";
        BuffDescStaging.Usage          = USAGE_STAGING;
        BuffDescStaging.BindFlags      = BIND_NONE;
        BuffDescStaging.Mode           = BUFFER_MODE_UNDEFINED;
        BuffDescStaging.CPUAccessFlags = CPU_ACCESS_READ;
        BuffDescStaging.Size           = sizeof(ParticleAttribs) * mNumParticles;

        m_pDevice->CreateBuffer( BuffDescStaging, nullptr, &mParticleAttribsStaging );
        VERIFY_EXPR( mParticleAttribsStaging != nullptr );

        FenceDesc FDesc;
        FDesc.Name = "ParticleAttribs available";
        FDesc.Type = FENCE_TYPE_GENERAL; // TODO: is this necessary? MeshShaders tutorial did not use it
        m_pDevice->CreateFence( FDesc, &mFenceParticleAttribsAvailable );
    }
#endif

    BuffDesc.ElementByteStride = sizeof(int);
    BuffDesc.Mode              = BUFFER_MODE_FORMATTED;
    BuffDesc.Size              = Uint64{BuffDesc.ElementByteStride} * static_cast<Uint64>( mNumParticles );
    BuffDesc.BindFlags         = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
    m_pDevice->CreateBuffer( BuffDesc, nullptr, &mParticleListHeadsBuffer );
    m_pDevice->CreateBuffer( BuffDesc, nullptr, &mParticleListsBuffer );
    RefCntAutoPtr<IBufferView> pParticleListHeadsBufferUAV;
    RefCntAutoPtr<IBufferView> pParticleListsBufferUAV;
    RefCntAutoPtr<IBufferView> pParticleListHeadsBufferSRV;
    RefCntAutoPtr<IBufferView> pParticleListsBufferSRV;
    {
        BufferViewDesc ViewDesc;
        ViewDesc.ViewType             = BUFFER_VIEW_UNORDERED_ACCESS;
        ViewDesc.Format.ValueType     = VT_INT32;
        ViewDesc.Format.NumComponents = 1;
        mParticleListHeadsBuffer->CreateView( ViewDesc, &pParticleListHeadsBufferUAV );
        mParticleListsBuffer->CreateView( ViewDesc, &pParticleListsBufferUAV );

        ViewDesc.ViewType = BUFFER_VIEW_SHADER_RESOURCE;
        mParticleListHeadsBuffer->CreateView( ViewDesc, &pParticleListHeadsBufferSRV );
        mParticleListsBuffer->CreateView( ViewDesc, &pParticleListsBufferSRV );
    }

    mResetParticleListsSRB.Release();
    mResetParticleListsPSO->CreateShaderResourceBinding( &mResetParticleListsSRB, true );
    mResetParticleListsSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "ParticleListHead")->Set( pParticleListHeadsBufferUAV );

    mRenderParticleSRB.Release();
    mRenderParticlePSO->CreateShaderResourceBinding( &mRenderParticleSRB, true );
    mRenderParticleSRB->GetVariableByName( SHADER_TYPE_VERTEX, "Particles" )->Set( pParticleAttribsBufferSRV );

    mMoveParticlesSRB.Release();
    mMoveParticlesPSO->CreateShaderResourceBinding( &mMoveParticlesSRB, true );
    mMoveParticlesSRB->GetVariableByName( SHADER_TYPE_COMPUTE, "Particles" )->Set( pParticleAttribsBufferUAV );
    mMoveParticlesSRB->GetVariableByName( SHADER_TYPE_COMPUTE, "ParticleListHead" )->Set( pParticleListHeadsBufferUAV );
    mMoveParticlesSRB->GetVariableByName( SHADER_TYPE_COMPUTE, "ParticleLists" )->Set( pParticleListsBufferUAV );

    mCollideParticlesSRB.Release();
    mCollideParticlesPSO->CreateShaderResourceBinding( &mCollideParticlesSRB, true );
    mCollideParticlesSRB->GetVariableByName( SHADER_TYPE_COMPUTE, "Particles" )->Set( pParticleAttribsBufferUAV );
    //mCollideParticlesSRB->GetVariableByName( SHADER_TYPE_COMPUTE, "ParticleListHead" )->Set( pParticleListHeadsBufferSRV );
    //mCollideParticlesSRB->GetVariableByName( SHADER_TYPE_COMPUTE, "ParticleLists" )->Set( pParticleListsBufferSRV );

    auto listHead =  mCollideParticlesSRB->GetVariableByName( SHADER_TYPE_COMPUTE, "ParticleListHead" );
    if( listHead ) {
        listHead->Set( pParticleListHeadsBufferSRV );
    }
    auto lists = mCollideParticlesSRB->GetVariableByName( SHADER_TYPE_COMPUTE, "ParticleLists" );
    if( lists ) {
        lists->Set( pParticleListsBufferSRV );
    }
}

void ComputeParticles::initConsantBuffer()
{
    BufferDesc BuffDesc;
    BuffDesc.Name           = "ParticleConstants buffer";
    BuffDesc.Usage          = USAGE_DYNAMIC;
    BuffDesc.BindFlags      = BIND_UNIFORM_BUFFER;
    BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    BuffDesc.Size           = sizeof(ParticleConstants);
    m_pDevice->CreateBuffer( BuffDesc, nullptr, &mParticleConstants );
}

void ComputeParticles::initCamera()
{
    mCamera.SetPos( float3{ 0, 0, -10 } );
    mCamera.SetLookAt( float3{ 0, 0, 1 } );
    mCamera.SetRotationSpeed( CameraRotationSpeed );
    mCamera.SetMoveSpeed( CameraMoveSpeed );
    mCamera.SetSpeedUpScales( CameraSpeedUp.x, CameraSpeedUp.y );
}

void ComputeParticles::watchShadersDir()
{
    std::filesystem::path shaderDir( "shaders/particles" );

    if( std::filesystem::exists( shaderDir ) ) {
        LOG_INFO_MESSAGE( __FUNCTION__, "| watching assets directory: ", shaderDir );
        try {
            ShadersDirWatchHandle = std::make_unique<FileWatchType>( shaderDir.string(),
                [=](const PathType &path, const filewatch::Event change_type ) {

                    // make a list of files we actually want to update if changed and check that here
                    const static std::vector<PathType> checkFilenames = {
                        "collide_particles.csh",
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
                            ShaderAssetsMarkedDirty = true;
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

void ComputeParticles::reloadOnAssetsUpdated()
{
    LOG_INFO_MESSAGE( __FUNCTION__, "| re-initializing shader assets" );

    initRenderParticlePSO();
    initUpdateParticlePSO();

    ShaderAssetsMarkedDirty = false;
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
}

void ComputeParticles::Update( double CurrTime, double ElapsedTime )
{
    SampleBase::Update( CurrTime, ElapsedTime );
    updateUI();

    if( ShaderAssetsMarkedDirty ) {
        reloadOnAssetsUpdated();
    }

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
    auto* rtv = m_pSwapChain->GetCurrentBackBufferRTV();
    auto* dsv = m_pSwapChain->GetDepthBufferDSV();
    // Clear the back buffer
    const float ClearColor[] = {0.350f, 0.350f, 0.350f, 1.0f};
    // Let the engine perform required state transitions
    m_pImmediateContext->ClearRenderTarget( rtv, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
    m_pImmediateContext->ClearDepthStencil( dsv, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

    if( mBackgroundCanvas && mDrawBackground ) {
        auto pixelConstants = mBackgroundCanvas->getPixelConstantsBuffer();
        MapHelper<BackgroundPixelConstants> cb( m_pImmediateContext, pixelConstants, MAP_WRITE, MAP_FLAG_DISCARD );
        cb->camPos = mCamera.GetPos();
        cb->camDir = mCamera.GetWorldAhead();

        auto swapChainDesc = m_pSwapChain->GetDesc();
        cb->resolution = float2( swapChainDesc.Width, swapChainDesc.Height );

        mBackgroundCanvas->render( m_pImmediateContext, mViewProjMatrix );
    }

    updateParticles();
    drawParticles();

    if( mTestSolid && mDrawTestSolid ) {
        mTestSolid->draw( m_pImmediateContext, mViewProjMatrix );
    }
}

void ComputeParticles::updateParticles()
{
    mProfiler->begin( m_pImmediateContext, "update particles" );

    // appears we always need to update this buffer or an assert failure happens (stale buffer)
    {
        // Map the buffer and write current world-view-projection matrix
        MapHelper<ParticleConstants> ConstData( m_pImmediateContext, mParticleConstants, MAP_WRITE, MAP_FLAG_DISCARD );
        ConstData->viewProj = mViewProjMatrix.Transpose();
        ConstData->numParticles = static_cast<Uint32>( mNumParticles );
        ConstData->deltaTime     = std::min( mTimeDelta, 1.f / 60.f) * mSimulationSpeed;
        ConstData->scale = mParticleScale;
        ConstData->gridSize = mGridSize;
    }

    if( mUpdateParticles ) {
        DispatchComputeAttribs dispatchAttribs;
        dispatchAttribs.ThreadGroupCountX = ( mNumParticles + mThreadGroupSize - 1) / mThreadGroupSize;

        m_pImmediateContext->SetPipelineState( mResetParticleListsPSO );
        m_pImmediateContext->CommitShaderResources( mResetParticleListsSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
        m_pImmediateContext->DispatchCompute( dispatchAttribs );

        m_pImmediateContext->SetPipelineState( mMoveParticlesPSO );
        m_pImmediateContext->CommitShaderResources( mMoveParticlesSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
        m_pImmediateContext->DispatchCompute( dispatchAttribs );

        m_pImmediateContext->SetPipelineState( mCollideParticlesPSO );
        m_pImmediateContext->CommitShaderResources( mCollideParticlesSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
        m_pImmediateContext->DispatchCompute( dispatchAttribs );

        m_pImmediateContext->SetPipelineState( mUpdateParticleSpeedPSO );
        // Use the same SRB
        m_pImmediateContext->CommitShaderResources( mCollideParticlesSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
        m_pImmediateContext->DispatchCompute( dispatchAttribs );
    }

#if DEBUG_PARTICLE_BUFFERS
    if( mDebugCopyParticles ) {
        DebugParticleData.resize( mNumParticles );

        m_pImmediateContext->CopyBuffer( mParticleAttribsBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
            mParticleAttribsStaging, 0, mNumParticles * sizeof(ParticleAttribs), RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

        // We should use synchronizations to safely access the mapped memory.
        // TODO: fix and re-enable this, but not crucial when looking at debug data
        //m_pImmediateContext->EnqueueSignal( mFenceParticleAttribsAvailable, mFenceParticleAttribsValue++ );
        //m_pImmediateContext->Flush(); // TODO: needed?

        //// TODO: check if this causes the main loop to wait
        //m_pImmediateContext->DeviceWaitForFence( mFenceParticleAttribsAvailable, mFenceParticleAttribsValue );

        {
            // TODO: should MapHelper<T> be type ParticleAttribs (take out the vector)?
            //MapHelper<std::vector<ParticleAttribs>> stagingData( m_pImmediateContext, mParticleAttribsBuffer, MAP_READ, MAP_FLAG_DO_NOT_WAIT );
            MapHelper<ParticleAttribs> stagingData( m_pImmediateContext, mParticleAttribsStaging, MAP_READ, MAP_FLAG_DO_NOT_WAIT );
            if( stagingData ) {
                //m_VisibleCubes =sStagingData[AvailableFrameId % m_StatisticsHistorySize].visibleCubes;
                // TODO: copy in one op to debug buffer
                for( size_t i = 0; i < mNumParticles; i++ ) {
                    const ParticleAttribs &p = stagingData[i];
                    DebugParticleData.at( i ) = p; 
                }
            }
        }
    }
#endif

    mProfiler->end( m_pImmediateContext, "update particles" );
}

void ComputeParticles::drawParticles()
{
    if( ! mDrawParticles ) {
        return;
    }

    mProfiler->begin( m_pImmediateContext, "draw particles" );

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

    mProfiler->end( m_pImmediateContext, "draw particles" );
}

// ------------------------------------------------------------------------------------------------------------
// ImGui
// ------------------------------------------------------------------------------------------------------------

void ComputeParticles::updateUI()
{
    im::SetNextWindowPos( ImVec2( 10, 10 ), ImGuiCond_FirstUseEver );
    if( im::Begin( "Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) ) {
        im::Checkbox( "profiling ui", &mProfilingUIEnabled );
        if( im::CollapsingHeader( "Particles", ImGuiTreeNodeFlags_DefaultOpen ) ) {
            im::Checkbox( "update", &mUpdateParticles );
            im::SameLine();
            im::Checkbox( "draw", &mDrawParticles );
#if DEBUG_PARTICLE_BUFFERS
            im::Checkbox( "debug particle data", &mDebugCopyParticles );
#endif

            if( im::InputInt( "count", &mNumParticles, 100, 1000, ImGuiInputTextFlags_EnterReturnsTrue ) ) {
                mNumParticles = std::min( std::max( mNumParticles, 100 ), 100000 );
                initParticleBuffers();
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
            }
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
    }
    im::End(); // Settings

#if DEBUG_PARTICLE_BUFFERS
    im::SetNextWindowPos( { 400, 20 }, ImGuiCond_FirstUseEver );
    im::SetNextWindowSize( { 800, 700 }, ImGuiCond_FirstUseEver );

    if( mDebugCopyParticles && im::Begin( "DebugCopyParticles", &mDebugCopyParticles ) ) {
        im::Text( "count: %d", mNumParticles );


        if( ! DebugParticleData.empty() ) {
            static int maxRows = 1000;

            static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Hideable;
            flags |= ImGuiTableFlags_ScrollY;
            flags |= ImGuiTableFlags_SizingFixedFit;

            if( im::BeginTable( "table_ParticleAttribs", 5, flags ) ) {
                ImGuiTableColumnFlags columnFlags = ImGuiTableColumnFlags_WidthFixed; 
                im::TableSetupScrollFreeze( 0, 1 ); // Make top row always visible
                im::TableSetupColumn( "index", columnFlags, 34 );
                im::TableSetupColumn( "pos", columnFlags, 180 );
                im::TableSetupColumn( "speed", columnFlags, 180 );
                im::TableSetupColumn( "collisions", ImGuiTableColumnFlags_None );
                im::TableSetupColumn( "temp", ImGuiTableColumnFlags_None );
                im::TableHeadersRow();

                ImGuiListClipper clipper;
				clipper.Begin( std::min( maxRows, mNumParticles ) );
				while( clipper.Step() ) {
					for( int row = clipper.DisplayStart; row<clipper.DisplayEnd; row++ ) {
						im::TableNextRow();
                        const auto &p = DebugParticleData.at( row );
                        int column = 0;
                        im::TableSetColumnIndex( column++ );
                        im::Text( "%d", row );
                        im::TableSetColumnIndex( column++ );
                        im::Text( "[%6.3f, %6.3f, %6.3f]", p.pos.x, p.pos.y, p.pos.z );
                        im::TableSetColumnIndex( column++ );
                        im::Text( "[%6.3f, %6.3f, %6.3f]", p.speed.x, p.speed.y, p.speed.z );
                        im::TableSetColumnIndex( column++ );
                        im::Text( " %d", p.numCollisions );
                        im::TableSetColumnIndex( column++ );
                        im::Text( "%0.03f", p.temperature );
                    }
				}
                im::EndTable();
            }
        }

        im::End(); // DebugCopyParticles
    }
#endif

    if( mProfilingUIEnabled && mProfiler ) {
        mProfiler->updateUI( &mProfilingUIEnabled );
    }
}