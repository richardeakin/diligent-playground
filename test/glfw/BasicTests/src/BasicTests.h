
#pragma once

#include "juniper/AppBasic.h"
#include "juniper/Juniper.h"
#include "juniper/Solids.h"

namespace dg = Diligent;

class BasicTests final : public ju::AppBasic {
public:
    virtual ~BasicTests();

    void prepareSettings( ju::AppSettings *settings ) override;
    void initialize() override;
    void resize( const dg::int2 &size ) override;
    void update( float deltaTime ) override;
    void draw() override;
    void keyEvent( const ju::KeyEvent &e ) override;
    void mouseEvent( const ju::MouseEvent &e ) override;

    const char* getTitle() const override  { return "BasicTests"; }

private:

    std::unique_ptr<ju::Solid>   mSolid;
};

