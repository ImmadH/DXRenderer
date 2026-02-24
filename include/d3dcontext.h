#pragma once 
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <stdexcept>

using Microsoft::WRL::ComPtr;


struct Device 
{
    ComPtr<IDXGIFactory6> dxgiFactory;
    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<ID3D12Device> d3d12Device;
};

//define the type of command queue (interface for submitting work)
struct D3DContextDesc 
{
    D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    D3D12_COMMAND_QUEUE_FLAGS flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    const wchar_t *name = L"MainQueue"; //give it a name for when we use a debugger
};

struct D3DContext 
{
    Device *device;
    D3DContextDesc contextDesc{};
    ComPtr<ID3D12CommandQueue> queue;
};

//enumerate for device and createn dx12 instance
void createDevice(Device *device);

//Create context and command queue
void createContext(D3DContext *context, Device *device);

//todo a delete context?