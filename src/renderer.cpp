#include "Renderer.h"
#include <stdexcept>
#include "theme.h"

static const Vertex kVerts[] =
{
    {{ 0.0f,  0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }},   // top   – red
    {{ 0.5f, -0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }},   // right – green
    {{-0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }},   // left  – blue
};


static const D3D12_INPUT_ELEMENT_DESC kInputLayout[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};



void initRenderer(Renderer* renderer, Application* app)
{
    renderer->app = app;
    createDevice(&renderer->device);


    renderer->context.contextDesc.type  = D3D12_COMMAND_LIST_TYPE_DIRECT;
    renderer->context.contextDesc.flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    renderer->context.contextDesc.name  = L"DirectQueue";
    createContext(&renderer->context, &renderer->device);


    createSwapChain(&renderer->swapChain, &renderer->context, app);

    initSync(&renderer->sync, &renderer->context);

    {
        HRESULT hr = renderer->sync.allocators[0]->Reset();
        if (FAILED(hr))
            throw std::runtime_error("initRenderer: allocator[0] reset failed.");

        hr = renderer->sync.commandList->Reset(
            renderer->sync.allocators[0].Get(), nullptr);
        if (FAILED(hr))
            throw std::runtime_error("initRenderer: command list reset failed.");

        MeshDesc meshDesc       = {};
        meshDesc.vertices       = kVerts;
        meshDesc.vertexCount    = static_cast<UINT>(_countof(kVerts));
        meshDesc.vertexStride   = sizeof(Vertex);

        createMesh(&renderer->mesh, &renderer->context,
                   renderer->sync.commandList.Get(), meshDesc);

        hr = renderer->sync.commandList->Close();
        if (FAILED(hr))
            throw std::runtime_error("initRenderer: command list close failed.");

        ID3D12CommandList* lists[] = { renderer->sync.commandList.Get() };
        renderer->context.queue->ExecuteCommandLists(1, lists);


        flushGPU(&renderer->sync, &renderer->context);


        renderer->mesh.uploadBuffer.Reset();
    }

    
    PipelineDesc pipeDesc         = {};
    pipeDesc.vsPath               = L"shaders/vertex.hlsl";
    pipeDesc.psPath               = L"shaders/pixel.hlsl";
    pipeDesc.inputLayout          = kInputLayout;
    pipeDesc.inputLayoutCount     = static_cast<UINT>(_countof(kInputLayout));
    pipeDesc.rtvFormat            = DXGI_FORMAT_R8G8B8A8_UNORM;
    createPipeline(&renderer->pipeline, &renderer->context, pipeDesc);

    //IMGUI
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = 1;
    heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    renderer->device.d3d12Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&renderer->imguiSrvHeap));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    applyOrangeTheme();
    ImGui::GetIO().FontGlobalScale = 2.0f;

    ImGui_ImplWin32_Init(app->hwnd);

    ImGui_ImplDX12_InitInfo initInfo = {};
    initInfo.Device             = renderer->device.d3d12Device.Get();
    initInfo.CommandQueue       = renderer->context.queue.Get();
    initInfo.NumFramesInFlight  = SwapChain::BufferCount;
    initInfo.RTVFormat          = DXGI_FORMAT_R8G8B8A8_UNORM;
    initInfo.DSVFormat          = DXGI_FORMAT_UNKNOWN;
    initInfo.SrvDescriptorHeap  = renderer->imguiSrvHeap.Get();
    initInfo.SrvDescriptorAllocFn = nullptr;
    initInfo.SrvDescriptorFreeFn  = nullptr;
    initInfo.LegacySingleSrvCpuDescriptor = renderer->imguiSrvHeap->GetCPUDescriptorHandleForHeapStart();
    initInfo.LegacySingleSrvGpuDescriptor = renderer->imguiSrvHeap->GetGPUDescriptorHandleForHeapStart();
    ImGui_ImplDX12_Init(&initInfo);
    
    renderer->viewport.TopLeftX = 0.0f;
    renderer->viewport.TopLeftY = 0.0f;
    renderer->viewport.Width    = static_cast<float>(app->width);
    renderer->viewport.Height   = static_cast<float>(app->height);
    renderer->viewport.MinDepth = 0.0f;
    renderer->viewport.MaxDepth = 1.0f;

    renderer->scissorRect.left   = 0;
    renderer->scissorRect.top    = 0;
    renderer->scissorRect.right  = static_cast<LONG>(app->width);
    renderer->scissorRect.bottom = static_cast<LONG>(app->height);
}


static void recordCommands(Renderer* renderer)
{
    ID3D12GraphicsCommandList* cmd = renderer->sync.commandList.Get();
    ID3D12Resource* bb  = getCurrentBackBuffer(&renderer->swapChain);
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = getCurrentRTV(&renderer->swapChain);

    
    D3D12_RESOURCE_BARRIER toRT = {};
    toRT.Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toRT.Transition.pResource = bb;
    toRT.Transition.StateBefore  = D3D12_RESOURCE_STATE_PRESENT;
    toRT.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    toRT.Transition.Subresource  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmd->ResourceBarrier(1, &toRT);

   
    const float clearColor[] = { 0.05f, 0.05f, 0.10f, 1.0f };
    cmd->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

    
    cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

   
    cmd->SetGraphicsRootSignature(renderer->pipeline.rootSignature.Get());
    cmd->SetPipelineState(renderer->pipeline.pipelineState.Get());

    
    cmd->RSSetViewports(1, &renderer->viewport);
    cmd->RSSetScissorRects(1, &renderer->scissorRect);
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    
    bindMesh(&renderer->mesh, cmd);
    cmd->DrawInstanced(renderer->mesh.vertexCount, 1, 0, 0);

    //IMGUI
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("DXRenderer");
    if (ImGui::Checkbox("Fullscreen", &renderer->app->fullScreen))
        toggleFullscreen(renderer->app);
    ImGui::End();

    ImGui::Render();

    ID3D12DescriptorHeap* heaps[] = { renderer->imguiSrvHeap.Get() };
    cmd->SetDescriptorHeaps(1, heaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd);

    
    D3D12_RESOURCE_BARRIER toPresent = {};
    toPresent.Type                        = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toPresent.Transition.pResource        = bb;
    toPresent.Transition.StateBefore      = D3D12_RESOURCE_STATE_RENDER_TARGET;
    toPresent.Transition.StateAfter       = D3D12_RESOURCE_STATE_PRESENT;
    toPresent.Transition.Subresource      = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmd->ResourceBarrier(1, &toPresent);
}


void renderFrame(Renderer* renderer)
{
    if (renderer->pendingResize)
    {
        flushGPU(&renderer->sync, &renderer->context);
        ImGui_ImplDX12_InvalidateDeviceObjects();

        resizeSwapChain(&renderer->swapChain, &renderer->context,
                        renderer->pendingWidth, renderer->pendingHeight);

        renderer->viewport.Width = (float)renderer->pendingWidth;
        renderer->viewport.Height = (float)renderer->pendingHeight;
        renderer->scissorRect.right  = (LONG)renderer->pendingWidth;
        renderer->scissorRect.bottom = (LONG)renderer->pendingHeight;
        renderer->app->width  = (int)renderer->pendingWidth;
        renderer->app->height = (int)renderer->pendingHeight;

        ImGui_ImplDX12_CreateDeviceObjects();
        renderer->pendingResize = false;
    }
    
    acquireNextImage(&renderer->swapChain);

    beginFrame(&renderer->sync);

    recordCommands(renderer);

    endFrame(&renderer->sync, &renderer->context);

    present(&renderer->swapChain);

    
    renderer->sync.frameIndex =
        (renderer->sync.frameIndex + 1) % Sync::FrameCount;
}


void shutdownRenderer(Renderer* renderer)
{
    flushGPU(&renderer->sync, &renderer->context);
    
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    renderer->imguiSrvHeap.Reset();

    destroyMesh(&renderer->mesh);

    renderer->pipeline.pipelineState.Reset();
    renderer->pipeline.rootSignature.Reset();

    for (UINT i = 0; i < SwapChain::BufferCount; ++i)
    renderer->swapChain.backBuffers[i].Reset();
    renderer->swapChain.rtvHeap.Reset();
    renderer->swapChain.swapChain.Reset();

    for (UINT i = 0; i < Sync::FrameCount; ++i)
    {
        renderer->sync.allocators[i].Reset();
        renderer->sync.fences[i].Reset();
    }
    renderer->sync.commandList.Reset();

    if (renderer->sync.fenceEvent)
    {
        CloseHandle(renderer->sync.fenceEvent);
        renderer->sync.fenceEvent = nullptr;
    }

    renderer->context.queue.Reset();
    renderer->device.d3d12Device.Reset();
    renderer->device.adapter.Reset();
    renderer->device.dxgiFactory.Reset();
}