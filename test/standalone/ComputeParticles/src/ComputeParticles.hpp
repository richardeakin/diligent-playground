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
    ComputeParticles();

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
    RefCntAutoPtr<dg::IBuffer>                mParticleConstantsBuffer;
    RefCntAutoPtr<dg::IBuffer>                mParticleAttribsBuffer;
    RefCntAutoPtr<dg::IBuffer>                mParticleListsBuffer;
    RefCntAutoPtr<dg::IBuffer>                mParticleListHeadsBuffer;

    // -------------------------------------------
    // Post Process
    void initPostProcessPSO();
    void downSample();
    void postProcess();

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
        //float3  fogColor = { 0.054f, 0.012f, 0.008f }; // bit more red
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
    dg::Uint64                              mFenceParticleAttribsValue = 1; // Can't signal 0
    bool                mDebugCopyParticles = true;
    std::vector<int>    mDebugParticleListsData;
    std::vector<int>    mDebugParticleListsHeadData;

    void buildDebugBinList( int nextParticle, std::vector<int> &allParticlesInBin );
    void debugPrintBinList( int bin );

#endif

    bool        mUIEnabled = true;  
    float       mParticleScaleVariation = 0.1f;
    float       mParticleBirthPadding = 0.1f;
    float       mSimulationSpeed    = 1.35f;
    float       mParticleSpeedVariation = 0.1f;
    int         mThreadGroupSize    = 256;
    float       mTime               = 0;
    float       mTimeDelta          = 0;
    bool        mDrawBackground     = true;
    bool        mDrawTestSolid      = false;
    bool        mDrawParticles      = true;
    bool        mUpdateParticles    = true;

    struct ParticleConstants {
        float4x4 viewProj;

        int     numParticles;
        float   time;
        float   deltaTime;
        float   separation;

        int3    gridSize;
        float   scale;

        float2  speedMinMax;
        float   alignment;
        float   cohesion;

        float   separationDist;
        float   alignmentDist;
        float   cohesionDist;
        float   sdfAvoidStrength;

        float3  worldMin;
        float   sdfAvoidDistance;

        float3  worldMax;
        float   padding2;
    };
    static_assert(sizeof(ParticleConstants) % 16 == 0, "must be aligned to 16 bytes");
    ParticleConstants mParticleConstants;

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
