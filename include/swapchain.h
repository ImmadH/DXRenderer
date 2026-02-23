#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

// swapchain creation, RTV heap + view creation, AcquireNextImage, Present
struct SwapChainDesc
{

};

struct SwapChain 
{
    ComPtr<IDXGISwapChain> swapChain;
};