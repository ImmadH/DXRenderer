#include "dx12.h"

bool InitD3D(Renderer* context,  Application* app)
{
    //enable and enumerate for gpu then create device
    HRESULT hr;
    IDXGIFactory4* dxgiFactory;
    hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hr))
    {
        return false;
    }

    IDXGIAdapter1* adapter;
    int adapterIndex = 0;
    bool adapterFound = false;

    while (dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            
            adapterIndex++; 
            continue;
        }

        hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
        if (SUCCEEDED(hr))
        {
            adapterFound = true;
            break;
        }

        adapterIndex++;
    }
    if (!adapterFound) 
    {
        return false;
    }
    
    hr = D3D12CreateDevice(
        adapter,
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&context->device)
    );
    if (FAILED(hr))
    {
        return false;
    }

    //create command queue
    D3D12_COMMAND_QUEUE_DESC cqDesc = {};

    hr = context->device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&context->commandQueue));
    if (FAILED(hr))
    {
        return false;
    }

    //create swapchain
    DXGI_MODE_DESC backBufferDesc = {};
    backBufferDesc.Width = app->WIDTH;
    backBufferDesc.Height = app->HEIGHT;
    backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;


    DXGI_SAMPLE_DESC sampleDesc = {};
    sampleDesc.Count = 1;

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = context->FRAME_BUFFER_COUNT;
    swapChainDesc.BufferDesc = backBufferDesc;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.OutputWindow = app->hwnd;
    swapChainDesc.SampleDesc = sampleDesc;
    swapChainDesc.Windowed = !app->fullScreen;

    IDXGISwapChain* tempSwapChain;
    dxgiFactory->CreateSwapChain(
        context->commandQueue,
        &swapChainDesc,
        &tempSwapChain
    );

    context->swapChain = static_cast<IDXGISwapChain3*>(tempSwapChain);
    context->frameIndex = context->swapChain->GetCurrentBackBufferIndex();


    //Back Buffers descriptor heap
    //rtv descriptor heap and create
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = context->FRAME_BUFFER_COUNT; // number of descriptors for this heap.
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // this heap is a render target view heap


    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = context->device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&context->rtvDescriptorHeap));
    if (FAILED(hr))
    {
        return false;
    }


    context->rtvDescriptorSize = context->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    //handle to the first descriptor
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(context->rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    //Create a RTV for each buffer
    for (int i = 0; i < context->FRAME_BUFFER_COUNT; i++)
    {
        hr = context->swapChain->GetBuffer(i, IID_PPV_ARGS(&context->renderTargets[i]));
        if (FAILED(hr))
        {
            return false;
        }

        context->device->CreateRenderTargetView(context->renderTargets[i], nullptr, rtvHandle);

        rtvHandle.Offset(1, context->rtvDescriptorSize);
    }

    //now create command allocators
    for (int i = 0; i < context->FRAME_BUFFER_COUNT; i++)
    {
        hr = context->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&context->commandAllocator[i]));
        if (FAILED(hr))
        {
            return false;
        }
    }
    //create command list - you would need to adjust this if we are doing multithreading
    hr = context->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, context->commandAllocator[0], NULL, IID_PPV_ARGS(&context->commandList));
    if (FAILED(hr))
    {
        return false;
    }
    // command lists are created in the recording state. our main loop will set it up for recording again so close it now
    context->commandList->Close();

    //synchronization

    for (int i = 0; i < context->FRAME_BUFFER_COUNT; i++)
    {
        hr = context->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&context->fence[i]));
        if (FAILED(hr))
        {
            return false;
        }
        context->fenceValue[i] = 0; // set the initial fence value to 0
    }

    // create a handle to a fence event
    context->fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (context->fenceEvent == nullptr)
    {
        return false;
    }

    return true;
}


void Update()
{
    //Updating application logic will go here
}

void UpdatePipeline(Renderer* context)
{
    //we will add commands to the command list
    HRESULT hr;
   
    WaitForPreviousFrame(context);


    hr = context->commandAllocator[context->frameIndex]->Reset();
    if (FAILED(hr))
    {
        context->Running = false;
    }
    
    hr = context->commandList->Reset(context->commandAllocator[context->frameIndex], NULL);
    if (FAILED(hr))
    {
        context->Running = false;
    }

    CD3DX12_RESOURCE_BARRIER barrierToRT = CD3DX12_RESOURCE_BARRIER::Transition(context->renderTargets[context->frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    context->commandList->ResourceBarrier(1, &barrierToRT);


    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(context->rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), context->frameIndex, context->rtvDescriptorSize);


    context->commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);


    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    context->commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);


    CD3DX12_RESOURCE_BARRIER barrierToPresent = CD3DX12_RESOURCE_BARRIER::Transition(context->renderTargets[context->frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    context->commandList->ResourceBarrier(1, &barrierToPresent);

    hr = context->commandList->Close();
    if (FAILED(hr))
    {
        context->Running = false;
    }

}

void Render(Renderer* context)
{
    HRESULT hr;
    UpdatePipeline(context); 

    ID3D12CommandList* ppCommandLists[] = { context->commandList };

    context->commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    hr = context->commandQueue->Signal(context->fence[context->frameIndex], context->fenceValue[context->frameIndex]);
    if (FAILED(hr))
    {
        context->Running = false;
    }

    // present the current backbuffer
    hr = context->swapChain->Present(0, 0);
    if (FAILED(hr))
    {
        context->Running = false;
    }

}

void Cleanup(Renderer* context)
{
    // wait for the gpu to finish all frames
    for (int i = 0; i < context->FRAME_BUFFER_COUNT; ++i)
    {
        context->frameIndex = i;
        WaitForPreviousFrame(context);
    }
    
    // get swapchain out of full screen before exiting
    BOOL fs = false;
    if (context->swapChain->GetFullscreenState(&fs, NULL))
        context->swapChain->SetFullscreenState(false, NULL);

    SAFE_RELEASE(context->device);
    SAFE_RELEASE(context->swapChain);
    SAFE_RELEASE(context->commandQueue);
    SAFE_RELEASE(context->rtvDescriptorHeap);
    SAFE_RELEASE(context->commandList);

    for (int i = 0; i < context->FRAME_BUFFER_COUNT; ++i)
    {
        SAFE_RELEASE(context->renderTargets[i]);
        SAFE_RELEASE(context->commandAllocator[i]);
        SAFE_RELEASE(context->fence[i]);
    };
}

//utility 
void WaitForPreviousFrame(Renderer* context)
{
    HRESULT hr;

    
    context->frameIndex = context->swapChain->GetCurrentBackBufferIndex();

    if (context->fence[context->frameIndex]->GetCompletedValue() < context->fenceValue[context->frameIndex])
    {

        hr = context->fence[context->frameIndex]->SetEventOnCompletion(context->fenceValue[context->frameIndex], context->fenceEvent);
        if (FAILED(hr))
        {
            context->Running = false;
        }

        WaitForSingleObject(context->fenceEvent, INFINITE);
    }

    // increment fenceValue for next frame
    context->fenceValue[context->frameIndex]++;
}