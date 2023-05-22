
#pragma once

#include "juniper/AppBasic.h"

namespace juniper {

using namespace Diligent;

class BasicTests final : public AppBasic {
public:
    virtual ~BasicTests();

    void prepareSettings( AppSettings *settings ) override;
    virtual void initialize() override;
    virtual void update(float dt) override;
    virtual void draw() override;
    virtual void keyEvent( const KeyEvent &key ) override;
    virtual void mouseEvent( float2 pos ) override;

    const char* getTitle() const override  { return "BasicTests"; }

private:

private:


};

} // namespace juniper
