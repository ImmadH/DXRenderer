#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "win32.h"

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

    Run();

    return 0;
}