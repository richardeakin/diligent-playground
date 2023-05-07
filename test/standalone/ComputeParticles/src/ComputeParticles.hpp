#pragma once

#include "SampleBase.hpp"
#include "ResourceMapping.h"
#include "BasicMath.hpp"
#include "FirstPersonCamera.hpp"

#include "common/src/Canvas.h"
#include "common/src/post/aa/FXAA.h"
#include "common/src/Profiler.h"
#include "Solids.h"

#define DEBUG_PARTICLE_BUFFERS 1

namespace dg = Diligent;
using dg::float3;
using dg::float4;
using dg::float4x4;
using dg::RefCntAutoPtr;

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

    RefCntAutoPtr<dg::IPipelineState>         mRenderParticlePSO;
    RefCntAutoPtr<dg::IShaderResourceBinding> mRenderParticleSRB;
    RefCntAutoPtr<dg::IPipelineState>         mResetParticleListsPSO;
    RefCntAutoPtr<dg::IShaderResourceBinding> mResetParticleListsSRB;
    RefCntAutoPtr<dg::IPipelineState>         mMoveParticlesPSO;
    RefCntAutoPtr<dg::IShaderResourceBinding> mMoveParticlesSRB;
    RefCntAutoPtr<dg::IPipelineState>         mInteractParticlesPSO;
    RefCntAutoPtr<dg::IShaderResourceBinding> mInteractParticlesSRB;
    RefCntAutoPtr<dg::IBuffer>                mParticleConstants;
    RefCntAutoPtr<dg::IBuffer>                mParticleAttribsBuffer;
    RefCntAutoPtr<dg::IBuffer>                mParticleListsBuffer;
    RefCntAutoPtr<dg::IBuffer>                mParticleListHeadsBuffer;

    // -------------------------------------------
    // Post Process
    void initPostProcessPSO();
    void DownSample();
    void PostProcess();

    RefCntAutoPtr<dg::IPipelineState>         mPostProcessPSO;
    RefCntAutoPtr<dg::IShaderResourceBinding> mPostProcessSRB;
    RefCntAutoPtr<dg::IBuffer>                mPostProcessConstantsBuffer;
    RefCntAutoPtr<dg::ITextureView>           mPostProcessRTV;

    RefCntAutoPtr<dg::IPipelineState>         mDownSamplePSO;
    static constexpr dg::Uint32               DownSampleFactor = 5;
    RefCntAutoPtr<dg::IShaderResourceBinding> mDownSampleSRB[DownSampleFactor];

    // Render to GBuffer
    struct GBuffer {
        RefCntAutoPtr<dg::ITextureView> ColorRTVs[DownSampleFactor];
        RefCntAutoPtr<dg::ITexture>     Color;
        RefCntAutoPtr<dg::ITextureView> ColorSRBs[DownSampleFactor];
        RefCntAutoPtr<dg::ITexture>     Depth;
    };
    GBuffer m_GBuffer;
    
    struct PostProcessConstants {
        float4x4    viewProjInv;

        float3  cameraPos;
        int     glowEnabled = 1;

        float3  fogColor = { 0.044f, 0.009f, 0.142f }; // dark purple
        int     fogEnabled = 1;
        float   fogIntensity = 0.092f;
        float3  padding;
    };
    static_assert(sizeof(PostProcessConstants) % 16 == 0, "must be aligned to 16 bytes");

    PostProcessConstants mPostProcessConstants;

    std::unique_ptr<ju::aa::FXAA>   mFXAA;
    bool                            mFXAAEnabled = true;

    // -------------------------------------------


#if DEBUG_PARTICLE_BUFFERS
    RefCntAutoPtr<dg::IBuffer>              mParticleAttribsStaging, mParticleListsStaging, mParticleListsHeadStaging;
    RefCntAutoPtr<dg::IFence>               mFenceParticleAttribsAvailable;
    dg::Uint64                                  mFenceParticleAttribsValue = 1; // Can't signal 0
    bool    mDebugCopyParticles = false;
#endif

    bool        mUIEnabled = true;  
    int         mNumParticles       = 200; // was: 1000
    float       mParticleScale      = 1.1f;
    float       mSeparation         = 1.9f;
    float       mAlignment          = 0.122f;
    float       mCohesion           = 0.051f;
    float       mSeparationDist     = 0.168f;
    float       mAlignmentDist      = 0.283f; 
    float       mCohesionDist       = 0.3f;
    float       mSimulationSpeed    = 0.75f;
    dg::float2  mSpeedMinMax        = { 0.01f, 1.1f };
    dg::int3    mGridSize           = { 10, 10, 10 };
    int         mThreadGroupSize    = 256;
    float       mTime               = 0;
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
