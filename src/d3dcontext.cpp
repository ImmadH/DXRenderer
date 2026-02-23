#include "d3dcontext.h"

void createDevice(Device *device)
{
    HRESULT hr;
    hr = CreateDXGIFactory1(IID_PPV_ARGS(&device->dxgiFactory));
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create DX12 Factory");
    }

    //for device enumeration we query for best adapter by asking windows
    for (UINT i = 0; ; ++i)
    {
        ComPtr<IDXGIAdapter1> adapter;

        if (device->dxgiFactory->EnumAdapterByGpuPreference(
            i, 
            DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            IID_PPV_ARGS(&device->adapter)) == DXGI_ERROR_NOT_FOUND)
        {
            break;
        }

        //skip software devices
        DXGI_ADAPTER_DESC1 adapterDesc{};
        adapter->GetDesc1(&adapterDesc);

        if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;
        
        //create device
        if (SUCCEEDED(D3D12CreateDevice(
                adapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(&device->d3d12Device))))
        {
            device->adapter = adapter;
            break;
        }
    }

    if (!device->d3d12Device)
        throw std::runtime_error("No D3D12 device found.");
}

void createContext(D3DContext *context, Device *device)
{

    //todo: probably check if a valid device/context is passed 
    
    context->device = device;

    //fill desc
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = context->contextDesc.type;
    desc.Flags = context->contextDesc.flags;


    HRESULT hr;
    hr = device->d3d12Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&context->queue));
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create a DX12 Command queue");
    }

    if (context->contextDesc.name)
    context->queue->SetName(context->contextDesc.name);
}