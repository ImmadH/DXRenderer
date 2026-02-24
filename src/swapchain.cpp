#include "swapchain.h"
#include "win32.h"
#include "D3DContext.h"

void createSwapChain(SwapChain *swapChain, const D3DContext *context, const Application *app)
{

    DXGI_SWAP_CHAIN_DESC1 swapDesc = {};
    swapDesc.Width = app->width;
    swapDesc.Height = app->height;
    swapDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapDesc.BufferCount = SwapChain::BufferCount;
    swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapDesc.SampleDesc.Count = 1; //this is for msaa

    ComPtr<IDXGISwapChain1> tempSwapChain;
    HRESULT hr = context->device->dxgiFactory->CreateSwapChainForHwnd(
        context->queue.Get(),
        app->hwnd,
        &swapDesc,
        nullptr,
        nullptr,
        &tempSwapChain
    );

    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create SwapChain");
    }

    //CreateSwapChainForHwnd returns an IDXGISwapChain1
    //this is base interface need to update
    hr = tempSwapChain.As(&swapChain->swapChain);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to update swapchain after creation");
    }

    //RTV create (the decriptors that point to render targets)
    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
    rtvDesc.NumDescriptors = SwapChain::BufferCount;
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    hr = context->device->d3d12Device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&swapChain->rtvHeap));
    if (FAILED(hr)) 
    {
        throw std::runtime_error("Failed to create RTV heap");
    }

    //store descriptor size 
    swapChain->rtvDescriptorSize = context->device->d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CPU_DESCRIPTOR_HANDLE handle = swapChain->rtvHeap->GetCPUDescriptorHandleForHeapStart();


    //Iterate the back buffers and create rtv
    for (UINT i = 0; i < SwapChain::BufferCount; ++i)
    {
        hr = swapChain->swapChain->GetBuffer(i, IID_PPV_ARGS(&swapChain->backBuffers[i]));
        if (FAILED(hr)) 
        {
            throw std::runtime_error("Failed to get swapchain back buffer");
        }

        context->device->d3d12Device->CreateRenderTargetView(
        swapChain->backBuffers[i].Get(), nullptr, handle);

        handle.ptr += swapChain->rtvDescriptorSize;
    }

    swapChain->currentBackBufferIndex = swapChain->swapChain->GetCurrentBackBufferIndex();
}

void acquireNextImage(SwapChain *swapChain)
{
    swapChain->currentBackBufferIndex = swapChain->swapChain->GetCurrentBackBufferIndex();
}

void present(SwapChain *swapChain)
{
    swapChain->swapChain->Present(1, 0);
}

ID3D12Resource *getCurrentBackBuffer(SwapChain *swapChain)
{
    return swapChain->backBuffers[swapChain->currentBackBufferIndex].Get();
}

void resizeSwapChain(SwapChain *swapChain, const D3DContext *context, UINT width, UINT height)
{
    for (UINT i = 0; i < SwapChain::BufferCount; ++i)
    {
        swapChain->backBuffers[i].Reset();
    }

    // Resize swapchain buffers
    HRESULT hr = swapChain->swapChain->ResizeBuffers(
        SwapChain::BufferCount,
        width,
        height,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        0
    );

    if (FAILED(hr))
    {
        throw std::runtime_error("ResizeBuffers failed.");
    }
        
    swapChain->rtvHeap.Reset();

    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
    rtvDesc.NumDescriptors = SwapChain::BufferCount;
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    hr = context->device->d3d12Device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&swapChain->rtvHeap));
    if (FAILED(hr))
        throw std::runtime_error("Failed to recreate RTV heap after resize.");

    swapChain->rtvDescriptorSize =
        context->device->d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    //get new back buffers and rebuild RTVs
    D3D12_CPU_DESCRIPTOR_HANDLE handle = swapChain->rtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < SwapChain::BufferCount; ++i)
    {
        hr = swapChain->swapChain->GetBuffer(i, IID_PPV_ARGS(&swapChain->backBuffers[i]));
        if (FAILED(hr))
            throw std::runtime_error("Failed to get swapchain back buffer after resize.");

        context->device->d3d12Device->CreateRenderTargetView(swapChain->backBuffers[i].Get(), nullptr, handle);
        handle.ptr += swapChain->rtvDescriptorSize;
    }

    swapChain->currentBackBufferIndex = swapChain->swapChain->GetCurrentBackBufferIndex();
}


D3D12_CPU_DESCRIPTOR_HANDLE getCurrentRTV(const SwapChain *swapChain)
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = swapChain->rtvHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += (SIZE_T)swapChain->currentBackBufferIndex * (SIZE_T)swapChain->rtvDescriptorSize;
    return handle;
}



