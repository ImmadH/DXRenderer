#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dx12.h>
#include "win32.h"
#include <DirectXMath.h>


#define SAFE_RELEASE(p) { if ((p)) { (p)->Release(); (p) = nullptr; } }

struct Renderer {
    static const int FRAME_BUFFER_COUNT = 3;
    ID3D12Device* device;
    IDXGISwapChain3* swapChain;
    ID3D12CommandQueue* commandQueue; //container for command lists
    ID3D12DescriptorHeap* rtvDescriptorHeap;
    ID3D12Resource* renderTargets[FRAME_BUFFER_COUNT]; //number of render targets equal to buffer count
    ID3D12CommandAllocator* commandAllocator[FRAME_BUFFER_COUNT]; 
    ID3D12GraphicsCommandList* commandList; // a command list we can record commands into, then execute them to render the frame
    ID3D12Fence* fence[FRAME_BUFFER_COUNT];
    //HANDLE to event when fence has completed work
    HANDLE fenceEvent;
    UINT64 fenceValue[FRAME_BUFFER_COUNT];
    //drawing related objects
    ID3D12PipelineState* pipelineStateObject;
    ID3D12RootSignature* rootSignature;
    D3D12_VIEWPORT viewport;
    D3D12_RECT scissorRect;
    ID3D12Resource* vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    bool Running = true;
    int frameIndex;
    int rtvDescriptorSize;
};

struct Vertex {
    DirectX::XMFLOAT3 pos;
};

bool InitD3D(Renderer* context, Application* app);
void Update();
void Render(Renderer* context);
void Cleanup(Renderer* context);
void WaitForPreviousFrame(Renderer* context);
void UpdatePipeline(Renderer* context);
bool InitPipeline(Renderer* context, Application* app);