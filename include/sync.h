#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <windows.h>
#include <cstdint>

using Microsoft::WRL::ComPtr;

struct D3DContext;
struct SwapChain;

struct Sync 
{
    static constexpr UINT FrameCount = 3;
    ComPtr<ID3D12CommandAllocator> allocators[FrameCount] = {};
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ComPtr<ID3D12Fence> fences[FrameCount] = {};
    uint64_t fenceValues[FrameCount] = {};
    HANDLE fenceEvent;
    UINT frameIndex = 0;
};

void initSync(Sync *sync, const D3DContext *context);
void beginFrame(Sync *sync);
void endFrame(Sync *sync, const D3DContext *context);
void flushGPU(Sync *sync, const D3DContext *context);