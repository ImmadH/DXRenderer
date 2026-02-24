#pragma once
#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct D3DContext;

//shader handling will also go here
struct PipelineDesc
{
    const wchar_t *vsPath;
    const wchar_t *psPath ;

    const D3D12_INPUT_ELEMENT_DESC *inputLayout;
    UINT inputLayoutCount = 0;

    DXGI_FORMAT rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
};

struct Pipeline
{
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pipelineState;
};

void createPipeline(Pipeline *pipeline,
                    const D3DContext *context,
                    const PipelineDesc &desc);