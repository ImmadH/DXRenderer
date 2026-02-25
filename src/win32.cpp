#include "win32.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include "Renderer.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool InitWindow(HINSTANCE hInstance, int ShowWnd, Application* app, WndProcContext* ctx)
{
    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = NULL;
    wc.cbWndExtra = NULL;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = app->windowName;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    //register the class calling the RegisterClassEx
    if (!RegisterClassEx(&wc))
    {
        MessageBox(NULL, L"Error registering class", L"ERROR", MB_OK | MB_ICONERROR);
        return false;
    }

    //window create
    app->hwnd = CreateWindowEx(NULL, 
                            app->windowName,
                            app->windowTitle,
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            app->width, app->height,
                            NULL, 
                            NULL, 
                            hInstance,
                            NULL);

    if (!app->hwnd) 
    {
        MessageBox(NULL, L"Error registering class", L"ERROR", MB_OK | MB_ICONERROR);
        return false; 
    }

    SetWindowLongPtr(app->hwnd, GWLP_USERDATA, (LONG_PTR)ctx);  

    ShowWindow(app->hwnd, ShowWnd);
    UpdateWindow(app->hwnd);
    
    return true;
}

//windows procedure callback
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;

    WndProcContext* ctx = (WndProcContext*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg)
    {
    case WM_SIZE:
        if (ctx && ctx->renderer && wParam != SIZE_MINIMIZED)
        {
            UINT w = LOWORD(lParam);
            UINT h = HIWORD(lParam);
            if (w > 0 && h > 0)
            {
                ctx->renderer->pendingResize = true;
                ctx->renderer->pendingWidth  = w;
                ctx->renderer->pendingHeight = h;
            }
        }
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
            DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}


void toggleFullscreen(Application* app)
{
    if (app->fullScreen)
    {
        GetWindowRect(app->hwnd, &app->windowedRect);

        // get the monitor the window is currently on
        MONITORINFO mi = {};
        mi.cbSize = sizeof(MONITORINFO);
        GetMonitorInfo(MonitorFromWindow(app->hwnd, MONITOR_DEFAULTTONEAREST), &mi);

        int x = mi.rcMonitor.left;
        int y = mi.rcMonitor.top;
        int w = mi.rcMonitor.right  - mi.rcMonitor.left;
        int h = mi.rcMonitor.bottom - mi.rcMonitor.top;

        SetWindowLong(app->hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(app->hwnd, HWND_TOP, x, y, w, h, SWP_FRAMECHANGED);
    }
    else
    {
        SetWindowLong(app->hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
        SetWindowPos(app->hwnd, nullptr,
            app->windowedRect.left,
            app->windowedRect.top,
            app->windowedRect.right  - app->windowedRect.left,
            app->windowedRect.bottom - app->windowedRect.top,
            SWP_FRAMECHANGED);
    }
}


void Run(Renderer*) {}