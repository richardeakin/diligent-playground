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

    dg::RefCntAutoPtr<dg::IPipelineState>         mRenderParticlePSO;
    dg::RefCntAutoPtr<dg::IShaderResourceBinding> mRenderParticleSRB;
    dg::RefCntAutoPtr<dg::IPipelineState>         mResetParticleListsPSO;
    dg::RefCntAutoPtr<dg::IShaderResourceBinding> mResetParticleListsSRB;
    dg::RefCntAutoPtr<dg::IPipelineState>         mMoveParticlesPSO;
    dg::RefCntAutoPtr<dg::IShaderResourceBinding> mMoveParticlesSRB;
    dg::RefCntAutoPtr<dg::IPipelineState>         mCollideParticlesPSO;
    dg::RefCntAutoPtr<dg::IShaderResourceBinding> mCollideParticlesSRB;
    dg::RefCntAutoPtr<dg::IPipelineState>         mUpdateParticleSpeedPSO;
    dg::RefCntAutoPtr<dg::IBuffer>                mConstants; // TODO: rename to mParticleConstants, in hlsl too
    dg::RefCntAutoPtr<dg::IBuffer>                mParticleAttribsBuffer;
    dg::RefCntAutoPtr<dg::IBuffer>                mParticleListsBuffer;
    dg::RefCntAutoPtr<dg::IBuffer>                mParticleListHeadsBuffer;
    //dg::RefCntAutoPtr<dg::IResourceMapping>       mResMapping;

    int     mNumParticles    = 2000;
    int     mThreadGroupSize = 256;
    float   mTimeDelta       = 0;
    float   mSimulationSpeed = 1;
    float   mParticleScale = 1;
    bool    mDrawBackground = true;
    bool    mDrawCube = true;
    bool    mDrawParticles = true;
    bool    mUpdateParticles = true;

    std::unique_ptr<ju::Canvas> mBackgroundCanvas;
    std::unique_ptr<ju::Cube>   mCube;

    dg::float4x4                mViewProjMatrix;
    dg::float4x4                mWorldViewProjMatrix; // TODO: get rid of this, no need for both

    dg::FirstPersonCamera mCamera;

    enum ParticleType {
        Sprite,
        Cube,
        Pyramid
    };

    ParticleType mParticleType = ParticleType::Sprite;

    void initCamera();

    void watchShadersDir();
    void reloadOnAssetsUpdated();

    void updateParticles();
    void drawParticles();
};
