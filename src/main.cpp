#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "win32.h"
#include "Renderer.h"
#include <stdexcept>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nShowCmd)
{
    ImGui_ImplWin32_EnableDpiAwareness();

    Application app;
    app.windowName  = L"DXRenderer";
    app.windowTitle = L"DXRenderer";
    app.width       = 1920;
    app.height      = 1080;
    app.fullScreen  = false;

    // context lives here so both InitWindow and the renderer can point at it
    WndProcContext ctx = {};
    ctx.app = &app;

    if (!InitWindow(hInstance, nShowCmd, &app, &ctx))
    {
        MessageBoxW(nullptr, L"Window initialisation failed.", L"Error",
                    MB_OK | MB_ICONERROR);
        return 1;
    }

    Renderer renderer = {};
    ctx.renderer = &renderer;   // wire renderer into WndProc context

    try
    {
        initRenderer(&renderer, &app);
    }
    catch (const std::exception& e)
    {
        MessageBoxA(nullptr, e.what(), "Renderer init failed", MB_OK | MB_ICONERROR);
        return 1;
    }

    RECT r;
    GetClientRect(app.hwnd, &r);
    PostMessage(app.hwnd, WM_SIZE, SIZE_RESTORED,
                MAKELPARAM(r.right, r.bottom));

    MSG msg = {};
    while (renderer.running)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                renderer.running = false;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            try
            {
                renderFrame(&renderer);
            }
            catch (const std::exception& e)
            {
                MessageBoxA(nullptr, e.what(), "Render error", MB_OK | MB_ICONERROR);
                renderer.running = false;
            }
        }
    }

    shutdownRenderer(&renderer);
    return 0;
}