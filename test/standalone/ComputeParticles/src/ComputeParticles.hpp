#pragma once

#include "SampleBase.hpp"
#include "ResourceMapping.h"
#include "BasicMath.hpp"
#include "FirstPersonCamera.hpp"

#include "juniper/Juniper.h"
#include "juniper/Canvas.h"
#include "juniper/post/aa/FXAA.h"
#include "juniper/Profiler.h"
//#include "juniper/Solids.h"
#include "SolidsOriginal.h"

#define DEBUG_PARTICLE_BUFFERS 1

namespace dg = Diligent;
using dg::int2;
using dg::int3;
using dg::int4;
using dg::float2;
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
    void drawBackgroundCanvas();
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

        float3  fogColor = { 0.034f, 0.003f, 0.022f }; // dark purple
        int     fogEnabled = 1;

        float   fogIntensity = 0.042f;
        float   glowIntensity = 1.0f;
        float   padding0;
        float   padding1;
    };
    static_assert(sizeof(PostProcessConstants) % 16 == 0, "must be aligned to 16 bytes");

    PostProcessConstants mPostProcessConstants;

    std::unique_ptr<juniper::post::FXAA>    mFXAA;
    bool                                    mFXAAEnabled = true;

    // -------------------------------------------


#if DEBUG_PARTICLE_BUFFERS
    RefCntAutoPtr<dg::IBuffer>              mParticleAttribsStaging, mParticleListsStaging, mParticleListsHeadStaging;
    RefCntAutoPtr<dg::IFence>               mFenceParticleAttribsAvailable;
    dg::Uint64                                  mFenceParticleAttribsValue = 1; // Can't signal 0
    bool    mDebugCopyParticles = false;
#endif

    bool        mUIEnabled = true;  
    int         mNumParticles       = 32;
    float       mParticleScale      = 0.5f;
    float       mParticleScaleVariation = 0.1f;
    float       mParticleBirthPadding = 0.1f;
    float       mSeparation         = 1.9f;
    float       mAlignment          = 0.25f;
    float       mCohesion           = 0.146f;
    float       mSeparationDist     = 0.688f;
    float       mAlignmentDist      = 1.692f; 
    float       mCohesionDist       = 1.956f;
    float       mSimulationSpeed    = 0.75f;
    float2      mSpeedMinMax        = { 0.01f, 4.0f };
    float       mParticleSpeedVariation = 0.1f;
    float3      mWorldMin           = { -10, 0.1f, -10 };
    float3      mWorldMax           = { 10, 10, 10 };
    int3        mGridSize           = { 10, 10, 10 };
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
