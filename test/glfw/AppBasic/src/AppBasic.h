
#pragma once

#include "juniper/AppGlfw.h"

namespace juniper {

using namespace Diligent;

class AppBasic : public AppGlfw {
public:
    virtual ~AppBasic();

    void prepareSettings( AppSettings *settings ) override;
    virtual void initialize() override;
    virtual void update(float dt) override;
    virtual void draw() override;
    virtual void keyEvent( const KeyEvent &key ) override;
    virtual void mouseEvent( float2 pos ) override;

    const char* getTitle() const override  { return "AppBasic"; }

private:

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
