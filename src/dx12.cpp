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
    // command list is created in the recording state. InitPipeline will use it to upload vertex data then close it.

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

    if (!InitPipeline(context, app))
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
    
    hr = context->commandList->Reset(context->commandAllocator[context->frameIndex], context->pipelineStateObject);
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

    // draw triangle
    context->commandList->SetGraphicsRootSignature(context->rootSignature);
    context->commandList->RSSetViewports(1, &context->viewport);
    context->commandList->RSSetScissorRects(1, &context->scissorRect);
    context->commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->commandList->IASetVertexBuffers(0, 1, &context->vertexBufferView);
    context->commandList->DrawInstanced(3, 1, 0, 0);

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
    SAFE_RELEASE(context->pipelineStateObject);
    SAFE_RELEASE(context->rootSignature);
    SAFE_RELEASE(context->vertexBuffer);

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

//used to create drawing related objects
bool InitPipeline(Renderer* context, Application* app)
{
    HRESULT hr;
    //ROOT SIGNATURE
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    ID3DBlob* signature;
    hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
    if (FAILED(hr))
    {
        return false;
    }

    hr = context->device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&context->rootSignature));
    if (FAILED(hr))
    {
        return false;
    }

    //SHADERS
    ID3DBlob* vertexShader;
    ID3DBlob* errorBuff;
    hr = D3DCompileFromFile(L"shaders/vertex.hlsl",
        nullptr,
        nullptr,
        "main",
        "vs_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &vertexShader,
        &errorBuff);

    if (FAILED(hr))
    {
        if (errorBuff) OutputDebugStringA((char*)errorBuff->GetBufferPointer());
        return false;
    }

    D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
    vertexShaderBytecode.BytecodeLength = vertexShader->GetBufferSize();
    vertexShaderBytecode.pShaderBytecode = vertexShader->GetBufferPointer();

    ID3DBlob* pixelShader;
    hr = D3DCompileFromFile(L"shaders/pixel.hlsl",
        nullptr,
        nullptr,
        "main",
        "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &pixelShader,
        &errorBuff);

    if (FAILED(hr))
    {
        if (errorBuff) OutputDebugStringA((char*)errorBuff->GetBufferPointer());
        return false;
    }

    D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
    pixelShaderBytecode.BytecodeLength = pixelShader->GetBufferSize();
    pixelShaderBytecode.pShaderBytecode = pixelShader->GetBufferPointer();


    //INPUT LAYOUT
    D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};

    inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
    inputLayoutDesc.pInputElementDescs = inputLayout;

    //PIPELINE STATE OBJECT
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = inputLayoutDesc;
    psoDesc.pRootSignature = context->rootSignature;
    psoDesc.VS = vertexShaderBytecode;
    psoDesc.PS = pixelShaderBytecode;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    //todo fix this, for multisample if needed but for now just one
    DXGI_SAMPLE_DESC sampleDesc = {};
    sampleDesc.Count = 1;
    //
    psoDesc.SampleDesc = sampleDesc;
    psoDesc.SampleMask = 0xffffffff;
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.NumRenderTargets = 1;

    hr = context->device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&context->pipelineStateObject));
    if (FAILED(hr))
    {   
        return false;
    }

    //VERTEX BUFFER - you will usually do, is use an upload heap to upload the vertex buffer to the GPU, the copy the data 
    //from the upload heap to a default heap. 
    //The default heap will stay in memory until we overwrite it or release it. 
    Vertex vList[] = {
        { { 0.0f, 0.5f, 0.5f } },
        { { 0.5f, -0.5f, 0.5f } },
        { { -0.5f, -0.5f, 0.5f } },
    };
    
    int vBufferSize = sizeof(vList);
    
    CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vBufferSize);
    context->device->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&context->vertexBuffer)
    );

    context->vertexBuffer->SetName(L"Vertex Buffer Resource Heap");

    ID3D12Resource* vBufferUploadHeap;
    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    context->device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vBufferUploadHeap)
    );

    vBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

    D3D12_SUBRESOURCE_DATA vertexData = {};
    vertexData.pData = reinterpret_cast<BYTE*>(vList);
    vertexData.RowPitch = vBufferSize;
    vertexData.SlicePitch = vBufferSize;
    UpdateSubresources(context->commandList, context->vertexBuffer, vBufferUploadHeap, 0, 0, 1, &vertexData);
    CD3DX12_RESOURCE_BARRIER vbBarrier = CD3DX12_RESOURCE_BARRIER::Transition(context->vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    context->commandList->ResourceBarrier(1, &vbBarrier);

    context->commandList->Close();
    ID3D12CommandList* ppCommandLists[] = { context->commandList };
    context->commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    context->fenceValue[context->frameIndex]++;

    hr = context->commandQueue->Signal(context->fence[context->frameIndex], context->fenceValue[context->frameIndex]);
    if (FAILED(hr))
    {
        context->Running = false;
    }

    context->vertexBufferView.BufferLocation = context->vertexBuffer->GetGPUVirtualAddress();
    context->vertexBufferView.StrideInBytes = sizeof(Vertex);
    context->vertexBufferView.SizeInBytes = vBufferSize;

    // Fill out the Viewport
    context->viewport.TopLeftX = 0;
    context->viewport.TopLeftY = 0;
    context->viewport.Width = static_cast<float>(app->WIDTH);
    context->viewport.Height = static_cast<float>(app->HEIGHT);
    context->viewport.MinDepth = 0.0f;
    context->viewport.MaxDepth = 1.0f;

    // Fill out a scissor rect
    context->scissorRect.left = 0;
    context->scissorRect.top = 0;
    context->scissorRect.right = app->WIDTH;
    context->scissorRect.bottom = app->HEIGHT;

    return true;
}