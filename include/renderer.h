#pragma once

#include "D3DContext.h"
#include "SwapChain.h"
#include "Sync.h"
#include "Pipeline.h"
#include "Mesh.h"
#include "win32.h"  
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h" 

struct Renderer
{
    Device     device;
    D3DContext context;
    SwapChain  swapChain;
    Sync       sync;
    Pipeline   pipeline;
    Mesh       mesh;

    //imgui needs a shader resource view descriptor
    ComPtr<ID3D12DescriptorHeap> imguiSrvHeap;

    Application* app;

    D3D12_VIEWPORT viewport    = {};
    D3D12_RECT     scissorRect = {};

    bool running = true;
    bool pendingResize  = false;
    UINT pendingWidth   = 0;  
    UINT pendingHeight  = 0;
};


void initRenderer(Renderer* renderer, Application* app);

void renderFrame(Renderer* renderer);

void shutdownRenderer(Renderer* renderer);