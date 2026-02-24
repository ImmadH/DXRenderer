#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

using Microsoft::WRL::ComPtr;

struct D3DContext;

struct Vertex
{
    float position[3];
    float color[3];
};

struct MeshDesc
{
    const void* vertices = nullptr;
    UINT vertexCount = 0;
    UINT vertexStride = sizeof(Vertex);
};

struct Mesh
{

    ComPtr<ID3D12Resource> vertexBuffer;

    //staging buffer
    ComPtr<ID3D12Resource> uploadBuffer;

    D3D12_VERTEX_BUFFER_VIEW vbView = {};

    UINT vertexCount = 0;
    UINT vertexStride = 0;
};


void createMesh(Mesh* mesh, const D3DContext* ctx, ID3D12GraphicsCommandList* cmdList, const MeshDesc& desc);

void bindMesh(const Mesh* mesh, ID3D12GraphicsCommandList* cmdList);

void destroyMesh(Mesh* mesh);