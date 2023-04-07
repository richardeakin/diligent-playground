
#include "ComputeParticles.hpp"
#include "BasicMath.hpp"
#include "MapHelper.hpp"
#include "imgui.h"
#include "ShaderMacroHelper.hpp"

#include "AppGlobal.h"
#include "../../common/src/FileWatch.hpp"

#include <filesystem>
#include <random>

using namespace Diligent;
namespace im = ImGui;

Diligent::SampleBase* Diligent::CreateSample()
{
    return new ComputeParticles();
}

namespace {

struct ParticleAttribs {
    float2 f2Pos;
    float2 f2NewPos;

    float2 f2Speed;
    float2 f2NewSpeed;

    float fSize          = 0;
    float fTemperature   = 0;
    int   iNumCollisions = 0;
    float fPadding0      = 0;
};

bool UseFirstPersonCamera = true;
bool RotateCube = true;

float CameraRotationSpeed = 0.005f;
float CameraMoveSpeed = 8.0f;
float2 CameraSpeedUp = { 0.2f, 10.0f }; // speed multipliers when {shift, ctrl} is down

std::unique_ptr<filewatch::FileWatch<std::filesystem::path>> ShadersDirWatchHandle;
bool                                                         ShaderAssetsMarkedDirty = false;

} // anon

void ComputeParticles::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    CreateConsantBuffer();
    CreateRenderParticlePSO();
    CreateUpdateParticlePSO();
    CreateParticleBuffers();

    auto global = app::global();
    global->renderDevice = m_pDevice;
    global->swapChainImageDesc = &m_pSwapChain->GetDesc();

    //RefCntAutoPtr<IShaderSourceInputStreamFactory> shaderSourceFactory;
    //m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &shaderSourceFactory);
    //global->shaderSourceFactory = shaderSourceFactory;

    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &global->shaderSourceFactory);

    mCube = std::make_unique<ju::Cube>();
    //mCube = std::make_unique<ju::Cube>( ju::VERTEX_COMPONENT_FLAG_POS_NORM_UV );
    initCamera();
}

void ComputeParticles::CreateRenderParticlePSO()
{
    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name = "Render particles PSO";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    // This tutorial will render to a single render target
    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = m_pSwapChain->GetDesc().ColorBufferFormat;
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = m_pSwapChain->GetDesc().DepthBufferFormat;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    // Disable back face culling TODO: enable for 3D
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    // Disable depth testing TODO: enable for 3D
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    auto& BlendDesc = PSOCreateInfo.GraphicsPipeline.BlendDesc;

    BlendDesc.RenderTargets[0].BlendEnable = True;
    BlendDesc.RenderTargets[0].SrcBlend    = BLEND_FACTOR_SRC_ALPHA;
    BlendDesc.RenderTargets[0].DestBlend   = BLEND_FACTOR_INV_SRC_ALPHA;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.Desc.UseCombinedTextureSamplers = true; // needed for OpenGL support

    // Create a shader source stream factory to load shaders from files.
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
    // Create particle vertex shader
    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Particle VS";
        ShaderCI.FilePath        = "particle.vsh";
        m_pDevice->CreateShader(ShaderCI, &pVS);
    }

    // Create particle pixel shader
    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Particle PS";
        ShaderCI.FilePath        = "particle.psh";
        m_pDevice->CreateShader(ShaderCI, &pPS);
    }

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    // Define variable type that will be used by default
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    // clang-format off
    // Shader variables should typically be mutable, which means they are expected
    // to change on a per-instance basis
    ShaderResourceVariableDesc Vars[] = 
    {
        {SHADER_TYPE_VERTEX, "g_Particles", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}
    };
    // clang-format on
    PSOCreateInfo.PSODesc.ResourceLayout.Variables    = Vars;
    PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Vars);

    m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pRenderParticlePSO);
    m_pRenderParticlePSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_Constants);
}

void ComputeParticles::CreateUpdateParticlePSO()
{
    ShaderCreateInfo ShaderCI;
    // Tell the system that the shader source code is in HLSL.
    // For OpenGL, the engine will convert this into GLSL under the hood.
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;

    // OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
    ShaderCI.Desc.UseCombinedTextureSamplers = true;

    // Create a shader source stream factory to load shaders from files.
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;

    ShaderMacroHelper Macros;
    Macros.AddShaderMacro("THREAD_GROUP_SIZE", m_ThreadGroupSize);
    Macros.Finalize();

    RefCntAutoPtr<IShader> pResetParticleListsCS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_COMPUTE;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Reset particle lists CS";
        ShaderCI.FilePath        = "reset_particle_lists.csh";
        ShaderCI.Macros          = Macros;
        m_pDevice->CreateShader(ShaderCI, &pResetParticleListsCS);
    }

    RefCntAutoPtr<IShader> pMoveParticlesCS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_COMPUTE;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Move particles CS";
        ShaderCI.FilePath        = "move_particles.csh";
        ShaderCI.Macros          = Macros;
        m_pDevice->CreateShader(ShaderCI, &pMoveParticlesCS);
    }

    RefCntAutoPtr<IShader> pCollideParticlesCS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_COMPUTE;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Collide particles CS";
        ShaderCI.FilePath        = "collide_particles.csh";
        ShaderCI.Macros          = Macros;
        m_pDevice->CreateShader(ShaderCI, &pCollideParticlesCS);
    }

    RefCntAutoPtr<IShader> pUpdatedSpeedCS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_COMPUTE;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Update particle speed CS";
        ShaderCI.FilePath        = "collide_particles.csh";
        Macros.AddShaderMacro("UPDATE_SPEED", 1);
        ShaderCI.Macros = Macros;
        m_pDevice->CreateShader(ShaderCI, &pUpdatedSpeedCS);
    }

    ComputePipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc&             PSODesc = PSOCreateInfo.PSODesc;

    // This is a compute pipeline
    PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;

    PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
    // clang-format off
    ShaderResourceVariableDesc Vars[] = 
    {
        {SHADER_TYPE_COMPUTE, "Constants", SHADER_RESOURCE_VARIABLE_TYPE_STATIC}
    };
    // clang-format on
    PSODesc.ResourceLayout.Variables    = Vars;
    PSODesc.ResourceLayout.NumVariables = _countof(Vars);

    PSODesc.Name      = "Reset particle lists PSO";
    PSOCreateInfo.pCS = pResetParticleListsCS;
    m_pDevice->CreateComputePipelineState(PSOCreateInfo, &m_pResetParticleListsPSO);
    m_pResetParticleListsPSO->GetStaticVariableByName(SHADER_TYPE_COMPUTE, "Constants")->Set(m_Constants);

    PSODesc.Name      = "Move particles PSO";
    PSOCreateInfo.pCS = pMoveParticlesCS;
    m_pDevice->CreateComputePipelineState(PSOCreateInfo, &m_pMoveParticlesPSO);
    m_pMoveParticlesPSO->GetStaticVariableByName(SHADER_TYPE_COMPUTE, "Constants")->Set(m_Constants);

    PSODesc.Name      = "Collidse particles PSO";
    PSOCreateInfo.pCS = pCollideParticlesCS;
    m_pDevice->CreateComputePipelineState(PSOCreateInfo, &m_pCollideParticlesPSO);
    m_pCollideParticlesPSO->GetStaticVariableByName(SHADER_TYPE_COMPUTE, "Constants")->Set(m_Constants);

    PSODesc.Name      = "Update particle speed PSO";
    PSOCreateInfo.pCS = pUpdatedSpeedCS;
    m_pDevice->CreateComputePipelineState(PSOCreateInfo, &m_pUpdateParticleSpeedPSO);
    m_pUpdateParticleSpeedPSO->GetStaticVariableByName(SHADER_TYPE_COMPUTE, "Constants")->Set(m_Constants);
}

void ComputeParticles::CreateParticleBuffers()
{
    m_pParticleAttribsBuffer.Release();
    m_pParticleListHeadsBuffer.Release();
    m_pParticleListsBuffer.Release();

    BufferDesc BuffDesc;
    BuffDesc.Name              = "Particle attribs buffer";
    BuffDesc.Usage             = USAGE_DEFAULT;
    BuffDesc.BindFlags         = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
    BuffDesc.Mode              = BUFFER_MODE_STRUCTURED;
    BuffDesc.ElementByteStride = sizeof(ParticleAttribs);
    BuffDesc.Size              = sizeof(ParticleAttribs) * m_NumParticles;

    std::vector<ParticleAttribs> ParticleData(m_NumParticles);

    std::mt19937 gen; // Standard mersenne_twister_engine. Use default seed
                      // to generate consistent distribution.

    std::uniform_real_distribution<float> pos_distr(-1.f, +1.f);
    std::uniform_real_distribution<float> size_distr(0.5f, 1.f);

    constexpr float fMaxParticleSize = 0.05f;
    float           fSize            = 0.7f / std::sqrt(static_cast<float>(m_NumParticles));
    fSize                            = std::min(fMaxParticleSize, fSize);
    for (auto& particle : ParticleData)
    {
        particle.f2NewPos.x   = pos_distr(gen);
        particle.f2NewPos.y   = pos_distr(gen);
        particle.f2NewSpeed.x = pos_distr(gen) * fSize * 5.f;
        particle.f2NewSpeed.y = pos_distr(gen) * fSize * 5.f;
        particle.fSize        = fSize * size_distr(gen);
    }

    BufferData VBData;
    VBData.pData    = ParticleData.data();
    VBData.DataSize = sizeof(ParticleAttribs) * static_cast<Uint32>(ParticleData.size());
    m_pDevice->CreateBuffer(BuffDesc, &VBData, &m_pParticleAttribsBuffer);
    IBufferView* pParticleAttribsBufferSRV = m_pParticleAttribsBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE);
    IBufferView* pParticleAttribsBufferUAV = m_pParticleAttribsBuffer->GetDefaultView(BUFFER_VIEW_UNORDERED_ACCESS);

    BuffDesc.ElementByteStride = sizeof(int);
    BuffDesc.Mode              = BUFFER_MODE_FORMATTED;
    BuffDesc.Size              = Uint64{BuffDesc.ElementByteStride} * static_cast<Uint64>(m_NumParticles);
    BuffDesc.BindFlags         = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
    m_pDevice->CreateBuffer(BuffDesc, nullptr, &m_pParticleListHeadsBuffer);
    m_pDevice->CreateBuffer(BuffDesc, nullptr, &m_pParticleListsBuffer);
    RefCntAutoPtr<IBufferView> pParticleListHeadsBufferUAV;
    RefCntAutoPtr<IBufferView> pParticleListsBufferUAV;
    RefCntAutoPtr<IBufferView> pParticleListHeadsBufferSRV;
    RefCntAutoPtr<IBufferView> pParticleListsBufferSRV;
    {
        BufferViewDesc ViewDesc;
        ViewDesc.ViewType             = BUFFER_VIEW_UNORDERED_ACCESS;
        ViewDesc.Format.ValueType     = VT_INT32;
        ViewDesc.Format.NumComponents = 1;
        m_pParticleListHeadsBuffer->CreateView(ViewDesc, &pParticleListHeadsBufferUAV);
        m_pParticleListsBuffer->CreateView(ViewDesc, &pParticleListsBufferUAV);

        ViewDesc.ViewType = BUFFER_VIEW_SHADER_RESOURCE;
        m_pParticleListHeadsBuffer->CreateView(ViewDesc, &pParticleListHeadsBufferSRV);
        m_pParticleListsBuffer->CreateView(ViewDesc, &pParticleListsBufferSRV);
    }

    m_pResetParticleListsSRB.Release();
    m_pResetParticleListsPSO->CreateShaderResourceBinding(&m_pResetParticleListsSRB, true);
    m_pResetParticleListsSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_ParticleListHead")->Set(pParticleListHeadsBufferUAV);

    m_pRenderParticleSRB.Release();
    m_pRenderParticlePSO->CreateShaderResourceBinding(&m_pRenderParticleSRB, true);
    m_pRenderParticleSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_Particles")->Set(pParticleAttribsBufferSRV);

    m_pMoveParticlesSRB.Release();
    m_pMoveParticlesPSO->CreateShaderResourceBinding(&m_pMoveParticlesSRB, true);
    m_pMoveParticlesSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_Particles")->Set(pParticleAttribsBufferUAV);
    m_pMoveParticlesSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_ParticleListHead")->Set(pParticleListHeadsBufferUAV);
    m_pMoveParticlesSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_ParticleLists")->Set(pParticleListsBufferUAV);

    m_pCollideParticlesSRB.Release();
    m_pCollideParticlesPSO->CreateShaderResourceBinding(&m_pCollideParticlesSRB, true);
    m_pCollideParticlesSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_Particles")->Set(pParticleAttribsBufferUAV);
    m_pCollideParticlesSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_ParticleListHead")->Set(pParticleListHeadsBufferSRV);
    m_pCollideParticlesSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_ParticleLists")->Set(pParticleListsBufferSRV);
}

void ComputeParticles::CreateConsantBuffer()
{
    BufferDesc BuffDesc;
    BuffDesc.Name           = "Constants buffer";
    BuffDesc.Usage          = USAGE_DYNAMIC;
    BuffDesc.BindFlags      = BIND_UNIFORM_BUFFER;
    BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    BuffDesc.Size           = sizeof(float4) * 2;
    m_pDevice->CreateBuffer(BuffDesc, nullptr, &m_Constants);
}

void ComputeParticles::initCamera()
{
    mCamera.SetPos( float3{ 0, 0, 10 } );
    mCamera.SetLookAt( float3{ 0, 0, -1 } );
    mCamera.SetRotationSpeed( CameraRotationSpeed );
    mCamera.SetMoveSpeed( CameraMoveSpeed );
    mCamera.SetSpeedUpScales( CameraSpeedUp.x, CameraSpeedUp.y );
}

void ComputeParticles::UpdateUI()
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (ImGui::InputInt("Num Particles", &m_NumParticles, 100, 1000, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            m_NumParticles = std::min(std::max(m_NumParticles, 100), 100000);
            CreateParticleBuffers();
        }
        ImGui::SliderFloat("Simulation Speed", &m_fSimulationSpeed, 0.1f, 5.f);

        if( im::CollapsingHeader( "Camera", ImGuiTreeNodeFlags_DefaultOpen ) ) {
            im::Checkbox( "enabled", &UseFirstPersonCamera );
            if( im::DragFloat( "move speed", &CameraMoveSpeed) ) {
                mCamera.SetMoveSpeed(CameraMoveSpeed);
            }
            if( im::DragFloat( "rotate speed", &CameraRotationSpeed) ) {
                mCamera.SetRotationSpeed(CameraRotationSpeed);
            }
            if( im::DragFloat2( "speed up scale", &CameraSpeedUp.x) ) {
                mCamera.SetSpeedUpScales(CameraSpeedUp.x, CameraSpeedUp.y);
            }
            if( im::Button("reset") ) {
                initCamera();
            }
        }
    }
    ImGui::End();
}

void ComputeParticles::ModifyEngineInitInfo(const ModifyEngineInitInfoAttribs& Attribs)
{
    SampleBase::ModifyEngineInitInfo(Attribs);

    Attribs.EngineCI.Features.ComputeShaders = DEVICE_FEATURE_STATE_ENABLED;
}

void ComputeParticles::WindowResize(Uint32 Width, Uint32 Height)
{
    LOG_INFO_MESSAGE("Terrain::WindowResize| size: [", Width, ", ", Height, "]" );

    // Update projection matrix.
    float AspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
    mCamera.SetProjAttribs(1.f, 1000.f, AspectRatio, PI_F / 4.f, m_pSwapChain->GetDesc().PreTransform, m_pDevice->GetDeviceInfo().IsGLDevice());
}

void ComputeParticles::Update(double CurrTime, double ElapsedTime)
{
    SampleBase::Update(CurrTime, ElapsedTime);
    UpdateUI();

    mCamera.Update(m_InputController, float(ElapsedTime));
    m_fTimeDelta = static_cast<float>(ElapsedTime);

    if( mCube ) {
        mCube->update( ElapsedTime );
    }

    // TODO: use proper camera
    {
        // Apply rotation
        float4x4 CubeModelTransform = float4x4::RotationY(static_cast<float>(CurrTime) * 1.0f) * float4x4::RotationX(-PI_F * 0.1f);

        // Camera is at (0, 0, -5) looking along the Z axis
        float4x4 View = float4x4::Translation(0.f, 0.0f, 5.0f);

        // Get pretransform matrix that rotates the scene according the surface orientation
        // TODO: understand when this is needed, not using with the FPS cam
        auto SrfPreTransform = GetSurfacePretransformMatrix(float3{0, 0, 1});

        // Get projection matrix adjusted to the current screen orientation
        auto Proj = GetAdjustedProjectionMatrix(PI_F / 4.0f, 0.1f, 100.f);

        // Compute world-view-projection matrix
        m_WorldViewProjMatrix = CubeModelTransform * View * SrfPreTransform * Proj;

        if( UseFirstPersonCamera ) {
            m_WorldViewProjMatrix = CubeModelTransform * mCamera.GetViewMatrix() * mCamera.GetProjMatrix();
        }
    }
}

// Render a frame
void ComputeParticles::Render()
{
    auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    auto* pDSV = m_pSwapChain->GetDepthBufferDSV();
    // Clear the back buffer
    const float ClearColor[] = {0.350f, 0.350f, 0.350f, 1.0f};
    // Let the engine perform required state transitions
    m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    {
        struct Constants
        {
            uint  uiNumParticles;
            float fDeltaTime;
            float fDummy0;
            float fDummy1;

            float2 f2Scale;
            int2   i2ParticleGridSize;
        };
        // Map the buffer and write current world-view-projection matrix
        MapHelper<Constants> ConstData(m_pImmediateContext, m_Constants, MAP_WRITE, MAP_FLAG_DISCARD);
        ConstData->uiNumParticles = static_cast<Uint32>(m_NumParticles);
        ConstData->fDeltaTime     = std::min(m_fTimeDelta, 1.f / 60.f) * m_fSimulationSpeed;

        float  AspectRatio = static_cast<float>(m_pSwapChain->GetDesc().Width) / static_cast<float>(m_pSwapChain->GetDesc().Height);
        float2 f2Scale     = float2(std::sqrt(1.f / AspectRatio), std::sqrt(AspectRatio));
        ConstData->f2Scale = f2Scale;

        int iParticleGridWidth          = static_cast<int>(std::sqrt(static_cast<float>(m_NumParticles)) / f2Scale.x);
        ConstData->i2ParticleGridSize.x = iParticleGridWidth;
        ConstData->i2ParticleGridSize.y = m_NumParticles / iParticleGridWidth;
    }

    DispatchComputeAttribs DispatAttribs;
    DispatAttribs.ThreadGroupCountX = (m_NumParticles + m_ThreadGroupSize - 1) / m_ThreadGroupSize;

    m_pImmediateContext->SetPipelineState(m_pResetParticleListsPSO);
    m_pImmediateContext->CommitShaderResources(m_pResetParticleListsSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->DispatchCompute(DispatAttribs);

    m_pImmediateContext->SetPipelineState(m_pMoveParticlesPSO);
    m_pImmediateContext->CommitShaderResources(m_pMoveParticlesSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->DispatchCompute(DispatAttribs);

    m_pImmediateContext->SetPipelineState(m_pCollideParticlesPSO);
    m_pImmediateContext->CommitShaderResources(m_pCollideParticlesSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->DispatchCompute(DispatAttribs);

    m_pImmediateContext->SetPipelineState(m_pUpdateParticleSpeedPSO);
    // Use the same SRB
    m_pImmediateContext->CommitShaderResources(m_pCollideParticlesSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->DispatchCompute(DispatAttribs);

    m_pImmediateContext->SetPipelineState(m_pRenderParticlePSO);
    m_pImmediateContext->CommitShaderResources(m_pRenderParticleSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    DrawAttribs drawAttrs;
    drawAttrs.NumVertices  = 4;
    drawAttrs.NumInstances = static_cast<Uint32>(m_NumParticles);
    m_pImmediateContext->Draw(drawAttrs);

    render3D();
}

void ComputeParticles::render3D()
{
    if( ! mCube ) {
        return;
    }

    mCube->render( m_pImmediateContext, m_WorldViewProjMatrix );
}

