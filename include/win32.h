#pragma once
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>

struct Application
{
    HWND    hwnd        = NULL;
    LPCTSTR windowName  = L"DXRenderer";
    LPCTSTR windowTitle = L"DXRenderer";
    int     width       = 1920;
    int     height      = 1080;
    bool    fullScreen  = false;
    RECT    windowedRect = {};
};

struct Renderer;
struct Sync;
struct D3DContext;
struct SwapChain;

struct WndProcContext
{
    Application* app      = nullptr;
    Renderer*    renderer = nullptr;
};

bool InitWindow(HINSTANCE hInstance, int ShowWnd, Application* app, WndProcContext* ctx);
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void toggleFullscreen(Application* app);
void Run(Renderer* context);