#pragma once
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>

struct Application {
    HWND hwnd = NULL;
    LPCTSTR windowName = L"DXRenderer";
    LPCTSTR windowTitle = L"DXRenderer";
    int width = 1920;
    int height = 1080;
    bool fullScreen = false;
};

bool InitWindow(HINSTANCE hInstance, int ShowWnd, Application* app);
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


//ignore
struct Renderer;
void Run(Renderer* context);

