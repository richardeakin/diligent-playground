
//#include "WinHPreface.h"
//#include <Windows.h>
//#include "WinHPostface.h"

#include "GraphicsTypes.h"
#include "imgui.h"
#include "ImGuiImplGlfw.h"
#include "backends/imgui_impl_glfw.h"
#include "DebugUtilities.hpp"

#include "GLFW/glfw3.h"

using namespace Diligent;

namespace juniper {

ImGuiImplGlfw::ImGuiImplGlfw( GLFWwindow*    window,
                              IRenderDevice* pDevice,
                              TEXTURE_FORMAT BackBufferFmt,
                              TEXTURE_FORMAT DepthBufferFmt,
                              Uint32         InitialVertexBufferSize,
                              Uint32         InitialIndexBufferSize )
    : ImGuiImplDiligent{ pDevice, BackBufferFmt, DepthBufferFmt, InitialVertexBufferSize, InitialIndexBufferSize }
{
    //ImGui_ImplWin32_Init(hWnd);

    ImGui_ImplGlfw_InitForOther( window, true );

    // TODO: is this called elsewhere already? and for DX12 / vulkan, etc
    //ImGui_ImplDX11_Init(DX11Common::GetDevice(), DX11Common::GetContext());
}

ImGuiImplGlfw::~ImGuiImplGlfw()
{
    ImGui_ImplGlfw_Shutdown();
}

void ImGuiImplGlfw::NewFrame( Uint32 RenderSurfaceWidth, Uint32 RenderSurfaceHeight, SURFACE_TRANSFORM SurfacePreTransform )
{
    VERIFY( SurfacePreTransform == SURFACE_TRANSFORM_IDENTITY, "Unexpected surface pre-transform" );

    ImGui_ImplGlfw_NewFrame();
    ImGuiImplDiligent::NewFrame( RenderSurfaceWidth, RenderSurfaceHeight, SurfacePreTransform );

#ifdef DILIGENT_DEBUG
    {
        ImGuiIO& io = ImGui::GetIO();
        VERIFY(io.DisplaySize.x == 0 || io.DisplaySize.x == static_cast<float>(RenderSurfaceWidth),
               "Render surface width (", RenderSurfaceWidth, ") does not match io.DisplaySize.x (", io.DisplaySize.x, ")");
        VERIFY(io.DisplaySize.y == 0 || io.DisplaySize.y == static_cast<float>(RenderSurfaceHeight),
               "Render surface height (", RenderSurfaceHeight, ") does not match io.DisplaySize.y (", io.DisplaySize.y, ")");
    }
#endif
}
#if 0
// Allow compilation with old Windows SDK. MinGW doesn't have default _WIN32_WINNT/WINVER versions.
#ifndef WM_MOUSEWHEEL
#    define WM_MOUSEWHEEL 0x020A
#endif
#ifndef WM_MOUSEHWHEEL
#    define WM_MOUSEHWHEEL 0x020E
#endif

LRESULT ImGuiImplWin32::Win32_ProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui::GetCurrentContext() == NULL)
        return 0;

    auto res = ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);

    ImGuiIO& io = ImGui::GetIO();
    switch (msg)
    {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONDBLCLK:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
            return io.WantCaptureMouse ? 1 : 0;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        case WM_CHAR:
            return io.WantCaptureKeyboard ? 1 : 0;
        case WM_SETCURSOR:
            return res;
    }

    return res;
}

#endif

} // namespace juniper
