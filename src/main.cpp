#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif

#include "win32.h"
#include "dx12.h"
//TODO 
//init d3d
int WINAPI WinMain(HINSTANCE hInstance,    
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nShowCmd)
{
    Application app;
    if (!InitWindow(hInstance, nShowCmd, &app))
    {
        MessageBox(0, L"Window Init FAILED", L"Error", MB_OK);
        return 0;
    }
    Renderer context;
    if (!InitD3D(&context, &app))
    {
        MessageBox(0, L"Failed to initialize direct3d 12",
            L"Error", MB_OK);
        Cleanup(&context);
        return 1;
    }


    Run(&context);
    WaitForPreviousFrame(&context);
    CloseHandle(context.fenceEvent);
    Cleanup(&context);

    return 0;
}