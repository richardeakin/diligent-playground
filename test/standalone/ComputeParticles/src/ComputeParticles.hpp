#pragma once

#include "SampleBase.hpp"
#include "ResourceMapping.h"
#include "BasicMath.hpp"
#include "FirstPersonCamera.hpp"

#include "common/src/Canvas.h"
#include "common/src/Profiler.h"
#include "Solids.h"

#define DEBUG_PARTICLE_BUFFERS 1

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
    void initRenderParticlePSO();
    void initUpdateParticlePSO();
    void initParticleBuffers();
    void initConsantBuffer();
    void initCamera();
    void initSolids();
    void updateUI();

    void watchShadersDir();
    void reloadOnAssetsUpdated();

    void updateParticles();
    void drawParticles();
    void updateDebugParticleDataUI();

    dg::RefCntAutoPtr<dg::IPipelineState>         mRenderParticlePSO;
    dg::RefCntAutoPtr<dg::IShaderResourceBinding> mRenderParticleSRB;
    dg::RefCntAutoPtr<dg::IPipelineState>         mResetParticleListsPSO;
    dg::RefCntAutoPtr<dg::IShaderResourceBinding> mResetParticleListsSRB;
    dg::RefCntAutoPtr<dg::IPipelineState>         mMoveParticlesPSO;
    dg::RefCntAutoPtr<dg::IShaderResourceBinding> mMoveParticlesSRB;
    dg::RefCntAutoPtr<dg::IPipelineState>         mInteractParticlesPSO;
    dg::RefCntAutoPtr<dg::IShaderResourceBinding> mInteractParticlesSRB;
    dg::RefCntAutoPtr<dg::IBuffer>                mParticleConstants;
    dg::RefCntAutoPtr<dg::IBuffer>                mParticleAttribsBuffer;
    dg::RefCntAutoPtr<dg::IBuffer>                mParticleListsBuffer;
    dg::RefCntAutoPtr<dg::IBuffer>                mParticleListHeadsBuffer;

#if DEBUG_PARTICLE_BUFFERS
    dg::RefCntAutoPtr<dg::IBuffer>              mParticleAttribsStaging, mParticleListsStaging, mParticleListsHeadStaging;
    dg::RefCntAutoPtr<dg::IFence>               mFenceParticleAttribsAvailable;
    dg::Uint64                                  mFenceParticleAttribsValue = 1; // Can't signal 0
    bool    mDebugCopyParticles = false;
#endif

    bool    mUIEnabled = true;  
    int     mNumParticles    = 100; // was: 1000
    float   mParticleScale   = 1;
    float   mZoneRadius      = 0.5f;
    float   mSimulationSpeed = 0.5f;
    dg::int3 mGridSize       = { 10, 10, 10 };
    int     mThreadGroupSize = 256;
    float   mTimeDelta       = 0;
    bool    mDrawBackground = true;
    bool    mDrawTestSolid = false;
    bool    mDrawParticles = true;
    bool    mUpdateParticles = true;


    std::unique_ptr<ju::Canvas> mBackgroundCanvas;
    std::unique_ptr<ju::Solid>   mTestSolid, mParticleSolid;

    dg::float4x4                mViewProjMatrix;

    dg::FirstPersonCamera mCamera;

    enum ParticleType {
        Sprite,
        Cube,
        Pyramid
    };

    ParticleType mParticleType = ParticleType::Pyramid;

    std::unique_ptr<ju::Profiler>   mProfiler;
    bool                            mProfilingUIEnabled = true;
};
