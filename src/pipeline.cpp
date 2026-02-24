#include "Pipeline.h"
#include "D3DContext.h"
#include "d3dx12.h"
#include <stdexcept>

static ComPtr<ID3DBlob> compileShaderFromFile(
    const wchar_t *path,
    const char *entry,
    const char *target)
{
    if (!path) 
    {
        throw std::runtime_error("Shader path is null.");
    }

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;

    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> errors;

    HRESULT hr = D3DCompileFromFile(
        path,
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entry,
        target,
        flags,
        0,
        &blob,
        &errors
    );

    if (FAILED(hr))
    {
        if (errors)
        {
            const char *msg = (const char*)errors->GetBufferPointer();
            throw std::runtime_error(msg ? msg : "Shader compile failed.");
        }
        throw std::runtime_error("Shader compile failed.");
    }

    return blob;
}

void createPipeline(Pipeline* pipeline, const D3DContext* context, const PipelineDesc& desc)
{

    ID3D12Device *device = context->device->d3d12Device.Get();

    ComPtr<ID3DBlob> vs = compileShaderFromFile(desc.vsPath, "main", "vs_5_0");
    ComPtr<ID3DBlob> ps = compileShaderFromFile(desc.psPath, "main", "ps_5_0");

    D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.NumParameters = 0;
    rsDesc.pParameters = nullptr;
    rsDesc.NumStaticSamplers = 0;
    rsDesc.pStaticSamplers = nullptr;
    rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> rsBlob;
    ComPtr<ID3DBlob> rsErrors;

    HRESULT hr = D3D12SerializeRootSignature(
        &rsDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &rsBlob,
        &rsErrors
    );

    if (FAILED(hr))
    {
        if (rsErrors)
        {
            const char* msg = (const char*)rsErrors->GetBufferPointer();
            throw std::runtime_error(msg ? msg : "Root signature serialization failed.");
        }
        throw std::runtime_error("Root signature serialization failed.");
    }

    hr = device->CreateRootSignature(
        0,
        rsBlob->GetBufferPointer(),
        rsBlob->GetBufferSize(),
        IID_PPV_ARGS(&pipeline->rootSignature)
    );

    if (FAILED(hr))
        throw std::runtime_error("Failed to create root signature.");


    //pipeline 
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = pipeline->rootSignature.Get();

    psoDesc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
    psoDesc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };

    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;

    psoDesc.SampleMask = UINT_MAX;

    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psoDesc.InputLayout = { desc.inputLayout, desc.inputLayoutCount };

    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = desc.rtvFormat;

    psoDesc.SampleDesc.Count = 1;

    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline->pipelineState));
    if (FAILED(hr))
        throw std::runtime_error("Failed to create pipeline state.");
}