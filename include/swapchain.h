#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <windows.h>

using Microsoft::WRL::ComPtr;

struct Application;     
struct D3DContext;     

//maybe move SC/RTV desc here for consistency but for now fine

struct SwapChain
{
    ComPtr<IDXGISwapChain3> swapChain;

    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    UINT rtvDescriptorSize = 0;

    static constexpr UINT BufferCount = 3;
    ComPtr<ID3D12Resource> backBuffers[BufferCount]{};

    UINT currentBackBufferIndex = 0;
};

void createSwapChain(SwapChain *swapChain, const D3DContext *context, const Application *app);

void acquireNextImage(SwapChain *swapChain);
void present(SwapChain *swapChain);
void resizeSwapChain(SwapChain *swapChain, const D3DContext *context, UINT width, UINT height);


ID3D12Resource* getCurrentBackBuffer(SwapChain *swapChain);
D3D12_CPU_DESCRIPTOR_HANDLE getCurrentRTV(const SwapChain *swapChain);