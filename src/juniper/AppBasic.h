
#pragma once

#include "juniper/AppGlfw.h"

namespace juniper {

using namespace Diligent; // TODO: remove

class AppBasic : public AppGlfw {
public:
    virtual ~AppBasic();

    //void prepareSettings( AppSettings *settings ) override;
    void initialize() override;
    void updateEntry(float dt) override;
    void drawEntry() override;
    
    virtual void update( float dt )         {}
    virtual void draw()                     {}

    const char* getTitle() const override  { return "AppBasic"; }

    //! Clears the swapchain's rendertarget to specified color
    void clear( const float4 &color );

private:

    RefCntAutoPtr<IShaderSourceInputStreamFactory> m_pShaderSourceFactory; // TODO: store on AppGlobal instead. Or can fetch App globally.. undecided

    // TODO: store ImGuiImpl in AppGlfw.h?
    //ImGuiImplDiligent*            mImGui = nullptr;

    // TODO: use these (See SampleBase.cpp)
    float  mSmoothFPS         = 0;
    double mLastFPSTime        = 0;
    Uint32 mNumFramesRendered  = 0;
    Uint32 mCurrentFrameNumber = 0;

};

} // namespace juniper
