#pragma once

#include "SampleBase.hpp"
#include "ResourceMapping.h"
#include "BasicMath.hpp"

namespace Diligent
{

class ComputeParticles final : public SampleBase
{
public:
    virtual void ModifyEngineInitInfo(const ModifyEngineInitInfoAttribs& Attribs) override final;

    virtual void Initialize(const SampleInitInfo& InitInfo) override final;

    virtual void Render() override final;
    virtual void Update(double CurrTime, double ElapsedTime) override final;

    virtual const Char* GetSampleName() const override final { return "Tutorial14: Compute Shader"; }

private:
    void CreateRenderParticlePSO();
    void CreateUpdateParticlePSO();
    void CreateParticleBuffers();
    void CreateConsantBuffer();
    void UpdateUI();

    int m_NumParticles    = 2000;
    int m_ThreadGroupSize = 256;

    RefCntAutoPtr<IPipelineState>         m_pRenderParticlePSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pRenderParticleSRB;
    RefCntAutoPtr<IPipelineState>         m_pResetParticleListsPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pResetParticleListsSRB;
    RefCntAutoPtr<IPipelineState>         m_pMoveParticlesPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pMoveParticlesSRB;
    RefCntAutoPtr<IPipelineState>         m_pCollideParticlesPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pCollideParticlesSRB;
    RefCntAutoPtr<IPipelineState>         m_pUpdateParticleSpeedPSO;
    RefCntAutoPtr<IBuffer>                m_Constants;
    RefCntAutoPtr<IBuffer>                m_pParticleAttribsBuffer;
    RefCntAutoPtr<IBuffer>                m_pParticleListsBuffer;
    RefCntAutoPtr<IBuffer>                m_pParticleListHeadsBuffer;
    RefCntAutoPtr<IResourceMapping>       m_pResMapping;

    float m_fTimeDelta       = 0;
    float m_fSimulationSpeed = 1;
};

} // namespace Diligent
