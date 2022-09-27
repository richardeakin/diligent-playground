/*
 *  Copyright 2019-2022 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

// TODO Still:
// - render to buffer
// - enable MSAA
// - add tesselated terrain from tut 08

#include <filesystem>

#include "Terrain.hpp"
#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "TextureUtilities.h"
#include "CommonlyUsedStates.h"
#include "ShaderMacroHelper.hpp"

#include "../../common/src/FileWatch.hpp"
#include "../../common/src/TexturedCube.hpp"
#include "imgui.h"


using namespace Diligent;

namespace {

struct Constants
{
    float4x4 WorldViewProj;
    float4   ViewportSize;
    float    LineWidth;
};

static constexpr TEXTURE_FORMAT RenderTargetFormat = TEX_FORMAT_RGBA8_UNORM;
static constexpr TEXTURE_FORMAT DepthBufferFormat  = TEX_FORMAT_D32_FLOAT;

std::unique_ptr<filewatch::FileWatch<std::filesystem::path>> ShadersDirWatchHandle;
bool                                                         ShaderAssetsMarkedDirty = false;
} // anon

void Terrain::ModifyEngineInitInfo(const ModifyEngineInitInfoAttribs& Attribs)
{
    SampleBase::ModifyEngineInitInfo(Attribs);
    // In this tutorial we will be using off-screen depth-stencil buffer, so
    // we do not need the one in the swap chain.
    Attribs.SCDesc.DepthBufferFormat = TEX_FORMAT_UNKNOWN;
}

void Terrain::CreateCubePSO()
{
    // Create a shader source stream factory to load shaders from files.
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);

    TexturedCube::CreatePSOInfo CubePsoCI;
    CubePsoCI.pDevice              = m_pDevice;
    CubePsoCI.RTVFormat            = RenderTargetFormat;
    CubePsoCI.DSVFormat            = DepthBufferFormat;
    CubePsoCI.pShaderSourceFactory = pShaderSourceFactory;
    CubePsoCI.VSFilePath           = "shaders/cube.vsh";
    CubePsoCI.PSFilePath           = "shaders/cube.psh";
    CubePsoCI.Components           = TexturedCube::VERTEX_COMPONENT_FLAG_POS_UV;

    m_pCubePSO = TexturedCube::CreatePipelineState(CubePsoCI);


    // Create dynamic uniform buffer that will store our transformation matrix
    // Dynamic buffers can be frequently updated by the CPU
    CreateUniformBuffer(m_pDevice, sizeof(float4x4), "VS constants CB", &m_CubeVSConstants);

    // Since we did not explcitly specify the type for 'Constants' variable, default
    // type (SHADER_RESOURCE_VARIABLE_TYPE_STATIC) will be used. Static variables never
    // change and are bound directly through the pipeline state object.
    m_pCubePSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_CubeVSConstants);

    // Since we are using mutable variable, we must create a shader resource binding object
    // http://diligentgraphics.com/2016/03/23/resource-binding-model-in-diligent-engine-2-0/
    m_pCubePSO->CreateShaderResourceBinding(&m_pCubeSRB, true);
}

void Terrain::CreateRenderTargetPSO()
{
    GraphicsPipelineStateCreateInfo RTPSOCreateInfo;

    // Pipeline state name is used by the engine to report issues
    // It is always a good idea to give objects descriptive names
    // clang-format off
    RTPSOCreateInfo.PSODesc.Name                                  = "Render Target PSO";
    RTPSOCreateInfo.PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
    RTPSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    // Set render target format which is the format of the swap chain's color buffer
    RTPSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = m_pSwapChain->GetDesc().ColorBufferFormat;
    // Set depth buffer format which is the format of the swap chain's back buffer
    RTPSOCreateInfo.GraphicsPipeline.DSVFormat                    = m_pSwapChain->GetDesc().DepthBufferFormat;
    RTPSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    RTPSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_BACK;
    // Enable depth testing
    RTPSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = False; // TODO: set to true?


    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.UseCombinedTextureSamplers = true;

    // create a shader source stream factory to load shaders from file
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;

    // Create a vertex shader
    RefCntAutoPtr<IShader> pRTVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Render Target VS";
        ShaderCI.FilePath        = "shaders/rendertarget.vsh";
        m_pDevice->CreateShader(ShaderCI, &pRTVS);
    }

    ShaderMacroHelper Macros;
    //Macros.AddShaderMacro("TRANSFORM_UV", TransformUVCoords);
    ShaderCI.Macros = Macros;

    // Create a pixel shader
    RefCntAutoPtr<IShader> pRTPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Render Target PS";
        ShaderCI.FilePath        = "shaders/rendertarget.psh";

        m_pDevice->CreateShader(ShaderCI, &pRTPS);
    }

    RTPSOCreateInfo.pVS = pRTVS;
    RTPSOCreateInfo.pPS = pRTPS;

    // Define variable type that will be used by default
    RTPSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    // Shader variables should typically be mutable, which means they are expected
    // to change on a per-instance basis
    ShaderResourceVariableDesc Vars[] =
    {
        { SHADER_TYPE_PIXEL, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE }
    };

    RTPSOCreateInfo.PSODesc.ResourceLayout.Variables    = Vars;
    RTPSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Vars);

    // Define immutable sampler for g_Texture. Immutable samplers should be used whenever possible
    ImmutableSamplerDesc ImtblSamplers[] =
    {
        { SHADER_TYPE_PIXEL, "g_Texture", Sam_LinearClamp }
    };

    RTPSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    RTPSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

    m_pDevice->CreateGraphicsPipelineState(RTPSOCreateInfo, &m_pRTPSO);

    // Since we did not explcitly specify the type for Constants, default type
    // (SHADER_RESOURCE_VARIABLE_TYPE_STATIC) will be used. Static variables never change and are bound directly
    // to the pipeline state object.
    m_pRTPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "Constants")->Set(m_RTPSConstants);
}

void Terrain::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    // Create dynamic uniform buffer that will store our transformation matrix
    // Dynamic buffers can be frequently updated by the CPU
    {
        BufferDesc CBDesc;
        CBDesc.Name           = "RTPS constants CB";
        CBDesc.Size           = sizeof(float4) + sizeof(float2x2) * 2;
        CBDesc.Usage          = USAGE_DYNAMIC;
        CBDesc.BindFlags      = BIND_UNIFORM_BUFFER;
        CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        m_pDevice->CreateBuffer(CBDesc, nullptr, &m_RTPSConstants); // FIXME: previous needs to be destroyed first
    }

    CreateCubePSO();
    CreateRenderTargetPSO();

    // Load textured cube
    m_CubeVertexBuffer = TexturedCube::CreateVertexBuffer(m_pDevice, TexturedCube::VERTEX_COMPONENT_FLAG_POS_UV);
    m_CubeIndexBuffer  = TexturedCube::CreateIndexBuffer(m_pDevice);
    m_CubeTextureSRV       = TexturedCube::LoadTexture(m_pDevice, "raccoon.jpg")->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    m_pCubeSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(m_CubeTextureSRV);

    WatchShadersDir();
}

namespace {
const char* watchEventTypeToString( const filewatch::Event change_type )
{
    switch (change_type) {
        case filewatch::Event::added:			return "added";
        case filewatch::Event::removed:			return "removed";
        case filewatch::Event::modified:		return "modified";
        case filewatch::Event::renamed_old:		return "renamed_old";
        case filewatch::Event::renamed_new:		return "renamed_new";
        default: break;
    };

    return "(unknown)";
}
} // anonymous namespace


void Terrain::WatchShadersDir()
{
    // watch assets dir for shader changes
    std::filesystem::path shaderDir("shaders");

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

void Terrain::ReloadOnAssetsUpdated()
{
    LOG_INFO_MESSAGE( __FUNCTION__, "| boom" );

    // clear resources that may be reloaded for dev needs
    m_pRTPSO.Release();

    //CreateCubePSO();
    CreateRenderTargetPSO();

    ShaderAssetsMarkedDirty = false;
}

void Terrain::WindowResize(Uint32 Width, Uint32 Height)
{
    LOG_INFO_MESSAGE("Terrain::WindowResize| size: [", Width, ", ", Height, "]" );

    // Create window-size offscreen render target
    TextureDesc RTColorDesc;
    RTColorDesc.Name      = "Offscreen render target";
    RTColorDesc.Type      = RESOURCE_DIM_TEX_2D;
    RTColorDesc.Width     = m_pSwapChain->GetDesc().Width;
    RTColorDesc.Height    = m_pSwapChain->GetDesc().Height;
    RTColorDesc.MipLevels = 1;
    RTColorDesc.Format    = RenderTargetFormat;
    // The render target can be bound as a shader resource and as a render target
    RTColorDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
    // Define optimal clear value
    RTColorDesc.ClearValue.Format   = RTColorDesc.Format;
    RTColorDesc.ClearValue.Color[0] = 0.350f;
    RTColorDesc.ClearValue.Color[1] = 0.350f;
    RTColorDesc.ClearValue.Color[2] = 0.350f;
    RTColorDesc.ClearValue.Color[3] = 1.f;
    RefCntAutoPtr<ITexture> pRTColor;
    m_pDevice->CreateTexture(RTColorDesc, nullptr, &pRTColor);
    // Store the render target view
    m_pColorRTV = pRTColor->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);


    // Create window-size depth buffer
    TextureDesc RTDepthDesc = RTColorDesc;
    RTDepthDesc.Name        = "Offscreen depth buffer";
    RTDepthDesc.Format      = DepthBufferFormat;
    RTDepthDesc.BindFlags   = BIND_DEPTH_STENCIL;
    // Define optimal clear value
    RTDepthDesc.ClearValue.Format               = RTDepthDesc.Format;
    RTDepthDesc.ClearValue.DepthStencil.Depth   = 1;
    RTDepthDesc.ClearValue.DepthStencil.Stencil = 0;
    RefCntAutoPtr<ITexture> pRTDepth;
    m_pDevice->CreateTexture(RTDepthDesc, nullptr, &pRTDepth);
    // Store the depth-stencil view
    m_pDepthDSV = pRTDepth->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);

    // We need to release and create a new SRB that references new off-screen render target SRV
    m_pRTSRB.Release();
    m_pRTPSO->CreateShaderResourceBinding(&m_pRTSRB, true);

    // Set render target color texture SRV in the SRB
    m_pRTSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(pRTColor->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
}

void Terrain::Update(double CurrTime, double ElapsedTime)
{
    SampleBase::Update(CurrTime, ElapsedTime);
    UpdateUI();

    if(ShaderAssetsMarkedDirty) {
        ReloadOnAssetsUpdated();
    }

    m_fCurrentTime = static_cast<float>(CurrTime);

    // Apply rotation
    float4x4 CubeModelTransform = float4x4::RotationY(static_cast<float>(CurrTime) * 1.0f) * float4x4::RotationX(-PI_F * 0.1f);

    // Camera is at (0, 0, -5) looking along the Z axis
    float4x4 View = float4x4::Translation(0.f, 0.0f, 5.0f);

    // Get pretransform matrix that rotates the scene according the surface orientation
    auto SrfPreTransform = GetSurfacePretransformMatrix(float3{0, 0, 1});
    // We will have to transform UV coordinates when performing post-processing
    m_UVPreTransformMatrix = float2x2{SrfPreTransform.m00, SrfPreTransform.m01, SrfPreTransform.m10, SrfPreTransform.m11};

    // Get projection matrix adjusted to the current screen orientation
    auto Proj = GetAdjustedProjectionMatrix(PI_F / 4.0f, 0.1f, 100.f);

    // Compute world-view-projection matrix
    m_WorldViewProjMatrix = CubeModelTransform * View * SrfPreTransform * Proj;
}

void Terrain::Render()
{
    // Clear the offscreen render target and depth buffer
    const float ClearColor[] = {0.350f, 0.350f, 0.350f, 1.0f};
    m_pImmediateContext->SetRenderTargets(1, &m_pColorRTV, m_pDepthDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearRenderTarget(m_pColorRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearDepthStencil(m_pDepthDSV, CLEAR_DEPTH_FLAG, 1.0f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    {
        // Map the cube's constant buffer and fill it in with its model-view-projection matrix
        MapHelper<float4x4> CBConstants(m_pImmediateContext, m_CubeVSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        *CBConstants = m_WorldViewProjMatrix.Transpose();
    }

    {
        struct VSConstants
        {
            float Time;
            float Padding0;
            float Padding1;
            float Padding2;

            float2x2 UVPreTransform;
            float2x2 UVPreTransformInv;
        };
        // Map the render target PS constant buffer and fill it in with current time
        MapHelper<VSConstants> CBConstants(m_pImmediateContext, m_RTPSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        CBConstants->Time              = m_fCurrentTime;
        CBConstants->UVPreTransform    = m_UVPreTransformMatrix;
        CBConstants->UVPreTransformInv = m_UVPreTransformMatrix.Inverse();
    }

    // Bind vertex and index buffers
    IBuffer* pBuffs[] = { m_CubeVertexBuffer };
    m_pImmediateContext->SetVertexBuffers(0, 1, pBuffs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    m_pImmediateContext->SetIndexBuffer(m_CubeIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Set the cube's pipeline state
    m_pImmediateContext->SetPipelineState(m_pCubePSO);

    // Commit the cube shader's resources
    m_pImmediateContext->CommitShaderResources(m_pCubeSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Draw the cube
    DrawIndexedAttribs DrawAttrs;
    DrawAttrs.IndexType  = VT_UINT32; // Index type
    DrawAttrs.NumIndices = 36;
    DrawAttrs.Flags      = DRAW_FLAG_VERIFY_ALL; // Verify the state of vertex and index buffers
    m_pImmediateContext->DrawIndexed(DrawAttrs);

    auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    // Clear the default render target
    const float Zero[] = {0.0f, 0.0f, 0.0f, 1.0f};
    m_pImmediateContext->SetRenderTargets(1, &pRTV, m_pSwapChain->GetDepthBufferDSV(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearRenderTarget(pRTV, Zero, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Set the render target pipeline state
    m_pImmediateContext->SetPipelineState(m_pRTPSO);

    // Commit the render target shader's resources
    m_pImmediateContext->CommitShaderResources(m_pRTSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Draw the render target's vertices
    DrawAttribs RTDrawAttrs;
    RTDrawAttrs.NumVertices = 4;
    RTDrawAttrs.Flags       = DRAW_FLAG_VERIFY_ALL; // Verify the state of vertex and index buffers
    m_pImmediateContext->Draw(RTDrawAttrs);
}

void Terrain::UpdateUI()
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        //ImGui::SliderFloat("Line Width", &m_LineWidth, 1.f, 10.f);
        ImGui::Checkbox( "shaders dirty", &ShaderAssetsMarkedDirty );
    }
    ImGui::End();
}

namespace Diligent {

SampleBase* CreateSample()
{
    return new Terrain();
}

}
