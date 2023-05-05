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
    void initConsantBuffers();
    void initCamera();
    void initSolids();
    void updateUI();

    void watchShadersDir();
    void checkReloadOnAssetsUpdated();

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

    // -------------------------------------------
    // Post Process
    void initPostProcessPSO();
    //void DownSample();
    void PostProcess();

    dg::RefCntAutoPtr<dg::IPipelineState>         m_PostProcessPSO;
    dg::RefCntAutoPtr<dg::IShaderResourceBinding> m_PostProcessSRB;
    dg::RefCntAutoPtr<dg::IBuffer> m_PostProcessConstants;

    // TODO: enable when adding GLOW
    //RefCntAutoPtr<IPipelineState>         m_DownSamplePSO;
    static constexpr dg::Uint32               DownSampleFactor = 5;
    //RefCntAutoPtr<IShaderResourceBinding> m_DownSampleSRB[DownSampleFactor];

    // Render to GBuffer
    struct GBuffer {
        dg::RefCntAutoPtr<dg::ITextureView> ColorRTVs[DownSampleFactor];
        dg::RefCntAutoPtr<dg::ITexture>     Color;
        dg::RefCntAutoPtr<dg::ITextureView> ColorSRBs[DownSampleFactor];
        dg::RefCntAutoPtr<dg::ITexture>     Depth;
    };
    GBuffer m_GBuffer;
    

    bool mGlowEnabled               = false; // TODO: add to const buffer
    dg::float3 mFogColor            = { 0.73f, 0.65f, 0.59f };
    // -------------------------------------------


#if DEBUG_PARTICLE_BUFFERS
    dg::RefCntAutoPtr<dg::IBuffer>              mParticleAttribsStaging, mParticleListsStaging, mParticleListsHeadStaging;
    dg::RefCntAutoPtr<dg::IFence>               mFenceParticleAttribsAvailable;
    dg::Uint64                                  mFenceParticleAttribsValue = 1; // Can't signal 0
    bool    mDebugCopyParticles = false;
#endif

    bool        mUIEnabled = true;  
    int         mNumParticles       = 200; // was: 1000
    float       mParticleScale      = 0.5f;
    float       mSeparation         = 0.68f;
    float       mAlignment          = 0.065f;
    float       mCohesion           = 0.033f;
    float       mSeparationDist     = 0.168f;
    float       mAlignmentDist      = 0.289f; 
    float       mCohesionDist       = 0.5f;
    float       mSimulationSpeed    = 0.75f;
    dg::float2  mSpeedMinMax        = { 0.01f, 1.1f };
    dg::int3    mGridSize           = { 10, 10, 10 };
    int         mThreadGroupSize    = 256;
    float       mTimeDelta          = 0;
    bool        mDrawBackground     = true;
    bool        mDrawTestSolid      = false;
    bool        mDrawParticles      = true;
    bool        mUpdateParticles    = true;


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
