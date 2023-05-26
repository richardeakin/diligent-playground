
#pragma once

#include "juniper/AppGlfw.h"

namespace juniper {

namespace dg = Diligent;
using dg::Uint32;
using dg::float3;
using dg::float4x4;

class AppBasic : public AppGlfw {
public:
    virtual ~AppBasic();

    //void prepareSettings( AppSettings *settings ) override;
    //! Inits AppGlobal (can be overridden later), calls AppBasic::initialize()
    void initEntry() override;
    void updateEntry(float dt) override;
    void drawEntry() override;
    
    virtual void initialize()               {}
    virtual void update( float dt )         {}
    virtual void draw()                     {}

    const char* getTitle() const override  { return "AppBasic"; }

    //! Clears the swapchain's rendertarget to specified color, and sets the depth stencil buffer to the default value if \a clearDepthStencil = true
    void clear( const float4 &color, bool clearDepthStencil = true );
    //!
    float4x4 getAdjustedProjectionMatrix( float fov, float nearPlane, float farPlane ) const;
    //!
    float4x4 getSurfacePretransformMatrix( const float3& cameraViewAxis ) const;

private:

    //RefCntAutoPtr<IShaderSourceInputStreamFactory> m_pShaderSourceFactory; // TODO: store on AppGlobal instead. Or can fetch App globally.. undecided

    // TODO: use these (See SampleBase.cpp)
    float  mSmoothFPS         = 0;
    double mLastFPSTime        = 0;
    Uint32 mNumFramesRendered  = 0;
    Uint32 mCurrentFrameNumber = 0;

};

} // namespace juniper
