#pragma once

#include "SampleBase.hpp"
#include "ResourceMapping.h"
#include "BasicMath.hpp"

#include "Cube.h"

namespace dg = Diligent;

class ComputeParticles final : public dg::SampleBase {
public:
    virtual void ModifyEngineInitInfo(const ModifyEngineInitInfoAttribs& Attribs) override final;

    virtual void Initialize(const dg::SampleInitInfo& InitInfo) override final;

    virtual void Render() override final;
    virtual void Update(double CurrTime, double ElapsedTime) override final;

    virtual const dg::Char* GetSampleName() const override final { return "ComputeParticles Blarg"; }

private:
    void CreateRenderParticlePSO();
    void CreateUpdateParticlePSO();
    void CreateParticleBuffers();
    void CreateConsantBuffer();
    void UpdateUI();

    int m_NumParticles    = 2000;
    int m_ThreadGroupSize = 256;

    dg::RefCntAutoPtr<dg::IPipelineState>         m_pRenderParticlePSO;
    dg::RefCntAutoPtr<dg::IShaderResourceBinding> m_pRenderParticleSRB;
    dg::RefCntAutoPtr<dg::IPipelineState>         m_pResetParticleListsPSO;
    dg::RefCntAutoPtr<dg::IShaderResourceBinding> m_pResetParticleListsSRB;
    dg::RefCntAutoPtr<dg::IPipelineState>         m_pMoveParticlesPSO;
    dg::RefCntAutoPtr<dg::IShaderResourceBinding> m_pMoveParticlesSRB;
    dg::RefCntAutoPtr<dg::IPipelineState>         m_pCollideParticlesPSO;
    dg::RefCntAutoPtr<dg::IShaderResourceBinding> m_pCollideParticlesSRB;
    dg::RefCntAutoPtr<dg::IPipelineState>         m_pUpdateParticleSpeedPSO;
    dg::RefCntAutoPtr<dg::IBuffer>                m_Constants;
    dg::RefCntAutoPtr<dg::IBuffer>                m_pParticleAttribsBuffer;
    dg::RefCntAutoPtr<dg::IBuffer>                m_pParticleListsBuffer;
    dg::RefCntAutoPtr<dg::IBuffer>                m_pParticleListHeadsBuffer;
    dg::RefCntAutoPtr<dg::IResourceMapping>       m_pResMapping;

    float m_fTimeDelta       = 0;
    float m_fSimulationSpeed = 1;

    std::unique_ptr<ju::Cube>   mCube;

    void renderCube();

    float4x4                              m_WorldViewProjMatrix; // TODO: use proper camera instead
};
