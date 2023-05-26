
#pragma once

#include "ImGuiImplDiligent.hpp"

typedef struct GLFWwindow GLFWwindow;

namespace juniper {

using namespace Diligent;

class ImGuiImplGlfw final : public ImGuiImplDiligent
{
public:
    //ImGuiImplGlfw(HWND           hWnd,
    //               IRenderDevice* pDevice,
    //               TEXTURE_FORMAT BackBufferFmt,
    //               TEXTURE_FORMAT DepthBufferFmt,
    //               Uint32         InitialVertexBufferSize = ImGuiImplDiligent::DefaultInitialVBSize,
    //               Uint32         InitialIndexBufferSize  = ImGuiImplDiligent::DefaultInitialIBSize);

    ImGuiImplGlfw( GLFWwindow*    window,
                   IRenderDevice* pDevice,
                   TEXTURE_FORMAT BackBufferFmt,
                   TEXTURE_FORMAT DepthBufferFmt,
                   Uint32         InitialVertexBufferSize = ImGuiImplDiligent::DefaultInitialVBSize,
                   Uint32         InitialIndexBufferSize  = ImGuiImplDiligent::DefaultInitialIBSize);


    ~ImGuiImplGlfw();

    //LRESULT Win32_ProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // clang-format off
    ImGuiImplGlfw             (const ImGuiImplGlfw&)  = delete;
    ImGuiImplGlfw             (      ImGuiImplGlfw&&) = delete;
    ImGuiImplGlfw& operator = (const ImGuiImplGlfw&)  = delete;
    ImGuiImplGlfw& operator = (      ImGuiImplGlfw&&) = delete;
    // clang-format on

    virtual void NewFrame(Uint32 RenderSurfaceWidth, Uint32 RenderSurfaceHeight, SURFACE_TRANSFORM SurfacePreTransform) override final;
};

} // namespace juniper
