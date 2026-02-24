#include "sync.h"
#include "d3dcontext.h"

void initSync(Sync *sync, const D3DContext *context)
{
    HRESULT hr;
    //we need 3 allocators and 3 fences
    for (UINT i = 0; i < Sync::FrameCount; ++i)
    {
        hr = context->device->d3d12Device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&sync->allocators[i])
        );

        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create command allocator.");
        }
    }

    hr = context->device->d3d12Device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        sync->allocators[0].Get(),
        nullptr,
        IID_PPV_ARGS(&sync->commandList)
    );

    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create command list");
    }

    hr = sync->commandList->Close();
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to close command list after creation.");
    }

    //create fences
    for (UINT i = 0; i < Sync::FrameCount; ++i)
    {
        hr = context->device->d3d12Device->CreateFence(
            0,
            D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(&sync->fences[i])
        );

        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create fence.");
        }
            
        sync->fenceValues[i] = 1; 
    }
        
    sync->fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!sync->fenceEvent)
    {
        throw std::runtime_error("Failed to create fence event.");
    }
        
    sync->frameIndex = 0;

}


//utility for waiting on fence 
static void waitForFenceValue(ID3D12Fence *fence, uint64_t value, HANDLE fenceEvent)
{
    if (fence->GetCompletedValue() < value)
    {
        fence->SetEventOnCompletion(value, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }
}

void beginFrame(Sync *sync)
{
    const UINT i = sync->frameIndex;

    // Wait until GPU is done with this frame's allocator
    waitForFenceValue(sync->fences[i].Get(), sync->fenceValues[i], sync->fenceEvent);

    HRESULT hr = sync->allocators[i]->Reset();
    if (FAILED(hr))
        throw std::runtime_error("Failed to reset command allocator.");

    hr = sync->commandList->Reset(sync->allocators[i].Get(), nullptr);
    if (FAILED(hr))
        throw std::runtime_error("Failed to reset command list.");
}



void endFrame(Sync *sync, const D3DContext *context)
{
    const UINT i = sync->frameIndex;

    
    HRESULT hr = sync->commandList->Close();
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to close command list.");
    }
    
    ID3D12CommandList* lists[] = { sync->commandList.Get() };
    context->queue->ExecuteCommandLists(1, lists);

    //Signal fence for this frame
    const uint64_t signalValue = sync->fenceValues[i];
    hr = context->queue->Signal(sync->fences[i].Get(), signalValue);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to signal fence.");
    }


    sync->fenceValues[i] = signalValue + 1;
}

void flushGPU(Sync *sync, const D3DContext *context)
{
    for (UINT i = 0; i < Sync::FrameCount; ++i)
    {
        const uint64_t signalValue = sync->fenceValues[i];

        HRESULT hr = context->queue->Signal(sync->fences[i].Get(), signalValue);
        if (FAILED(hr))
            throw std::runtime_error("flushGPU: failed to signal fence.");

        waitForFenceValue(sync->fences[i].Get(), signalValue, sync->fenceEvent);

        sync->fenceValues[i] = signalValue + 1;
    }
}