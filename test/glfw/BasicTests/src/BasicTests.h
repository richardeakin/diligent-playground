
#pragma once

#include "juniper/AppBasic.h"
#include "juniper/Juniper.h"
#include "juniper/Solids.h"
#include "juniper/Camera.h"

namespace dg = Diligent;

class BasicTests final : public ju::AppBasic {
public:
    virtual ~BasicTests();

    void prepareSettings( ju::AppSettings *settings ) override;
    void initialize() override;
    void resize( const dg::int2 &size ) override;
    void update( float deltaTime ) override;
    void draw() override;
    void keyEvent( ju::KeyEvent &e ) override;
    void mouseEvent( ju::MouseEvent &e ) override;

    const char* getTitle() const override  { return "BasicTests"; }

private:

    void initCamera();
    void updateUI();

    std::unique_ptr<ju::Solid>   mSolid;

    ju::FlyCam     mCam;
};
