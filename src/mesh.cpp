#include "Mesh.h"
#include "D3DContext.h"

#include <stdexcept>
#include <cstring> 

static inline UINT64 alignUp(UINT64 value, UINT64 alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

void createMesh(Mesh* mesh,
                const D3DContext* ctx,
                ID3D12GraphicsCommandList* cmdList,
                const MeshDesc& desc)
{

    ID3D12Device* device = ctx->device->d3d12Device.Get();

    const UINT64 bufferSize = (UINT64)desc.vertexCount * (UINT64)desc.vertexStride;

    mesh->vertexCount  = desc.vertexCount;
    mesh->vertexStride = desc.vertexStride;


    D3D12_HEAP_PROPERTIES defaultHeap{};
    defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC vbDesc{};
    vbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    vbDesc.Width = bufferSize;
    vbDesc.Height = 1;
    vbDesc.DepthOrArraySize = 1;
    vbDesc.MipLevels = 1;
    vbDesc.SampleDesc.Count = 1;
    vbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &vbDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, 
        nullptr,
        IID_PPV_ARGS(&mesh->vertexBuffer)
    );

    if (FAILED(hr))
        throw std::runtime_error("createMesh: failed to create GPU vertex buffer.");


    D3D12_HEAP_PROPERTIES uploadHeap{};
    uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

    hr = device->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &vbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&mesh->uploadBuffer)
    );

    if (FAILED(hr))
        throw std::runtime_error("createMesh: failed to create upload buffer.");

 
    void* mapped = nullptr;
    hr = mesh->uploadBuffer->Map(0, nullptr, &mapped);
    if (FAILED(hr) || !mapped)
        throw std::runtime_error("createMesh: failed to map upload buffer.");

    std::memcpy(mapped, desc.vertices, (size_t)bufferSize);
    mesh->uploadBuffer->Unmap(0, nullptr);


    cmdList->CopyBufferRegion(
        mesh->vertexBuffer.Get(), 0,
        mesh->uploadBuffer.Get(), 0,
        bufferSize
    );

    // Transition to VB state for drawing
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = mesh->vertexBuffer.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    cmdList->ResourceBarrier(1, &barrier);


    mesh->vbView.BufferLocation = mesh->vertexBuffer->GetGPUVirtualAddress();
    mesh->vbView.SizeInBytes    = (UINT)bufferSize;
    mesh->vbView.StrideInBytes  = desc.vertexStride;

    
}

void bindMesh(const Mesh* mesh, ID3D12GraphicsCommandList* cmdList)
{
    if (!mesh || !cmdList)
        throw std::runtime_error("bindMesh: invalid arguments.");

    cmdList->IASetVertexBuffers(0, 1, &mesh->vbView);
}

void destroyMesh(Mesh* mesh)
{
    if (!mesh) return;

    mesh->uploadBuffer.Reset();
    mesh->vertexBuffer.Reset();
    mesh->vbView = {};
    mesh->vertexCount = 0;
    mesh->vertexStride = 0;
}