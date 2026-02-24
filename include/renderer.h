#pragma once

#include "D3DContext.h"
#include "SwapChain.h"
#include "Sync.h"
#include "Pipeline.h"
#include "Mesh.h"
#include "win32.h"   

struct Renderer
{
    Device     device;
    D3DContext context;
    SwapChain  swapChain;
    Sync       sync;
    Pipeline   pipeline;
    Mesh       mesh;

    D3D12_VIEWPORT viewport    = {};
    D3D12_RECT     scissorRect = {};

    bool running = true;
};


void initRenderer(Renderer* renderer, Application* app);

void renderFrame(Renderer* renderer);

void shutdownRenderer(Renderer* renderer);