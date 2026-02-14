#include "win32.h"
#include "dx12.h"


bool InitWindow(HINSTANCE hInstance, int ShowWnd, Application* app)
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
                            app->WIDTH, app->HEIGHT,
                            NULL, 
                            NULL, 
                            hInstance,
                            NULL);

    if (!app->hwnd) 
    {
        MessageBox(NULL, L"Error registering class", L"ERROR", MB_OK | MB_ICONERROR);
        return false; 
    }

    ShowWindow(app->hwnd, ShowWnd);
    UpdateWindow(app->hwnd);
    
    return true;
}

//windows procedure callback
LRESULT CALLBACK WndProc(HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (msg)
    {

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) 
        {
            DestroyWindow(hwnd);
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd,
        msg,
        wParam,
        lParam);
}


//GAME LOOP 
void Run(Renderer* context) {
    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));

    while (context->Running)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                break;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Update();
            Render(context);
        }
    }
}