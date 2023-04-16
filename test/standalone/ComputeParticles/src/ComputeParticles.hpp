#pragma once

#include "SampleBase.hpp"
#include "ResourceMapping.h"
#include "BasicMath.hpp"
#include "FirstPersonCamera.hpp"

#include "common/src/Canvas.h"
#include "Cube.h"

namespace dg = Diligent;

class ComputeParticles final : public dg::SampleBase {
public:
    virtual void ModifyEngineInitInfo(const ModifyEngineInitInfoAttribs& Attribs) override final;

    virtual void Initialize(const dg::SampleInitInfo& InitInfo) override final;
    virtual void WindowResize(dg::Uint32 Width, dg::Uint32 Height) override final;
    virtual void Update(double CurrTime, double ElapsedTime) override final;
    virtual void Render() override final;

    virtual const dg::Char* GetSampleName() const override final { return "ComputeParticles"; }

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

    float mParticleScale = 1;

    bool    mDrawBackground = true;
    bool    mDrawCube = true;
    bool    mDrawParticles = true;
    bool    mUpdateParticles = true;

    std::unique_ptr<ju::Canvas> mBackgroundCanvas;
    std::unique_ptr<ju::Cube>   mCube;

    dg::float4x4             m_ViewProjMatrix;
    dg::float4x4         m_WorldViewProjMatrix; // TODO: get rid of this, no need for both

    dg::FirstPersonCamera mCamera;

    void initCamera();

    void watchShadersDir();
    void reloadOnAssetsUpdated();

    void updateParticles();
    void drawParticles();
    void draw3D();
};
