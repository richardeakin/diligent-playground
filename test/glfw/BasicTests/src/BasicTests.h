
#pragma once

#include "juniper/AppBasic.h"
#include "juniper/Juniper.h"

namespace dg = Diligent;

class BasicTests final : public ju::AppBasic {
public:
    virtual ~BasicTests();

    void prepareSettings( ju::AppSettings *settings ) override;
    virtual void initialize() override;
    virtual void update( float deltaTime ) override;
    virtual void draw() override;
    virtual void keyEvent( const ju::KeyEvent &key ) override;
    virtual void mouseEvent( dg::float2 pos ) override;

    const char* getTitle() const override  { return "BasicTests"; }

private:

};

