#pragma once
#include "DXCommon.h"
#include "externals/DirectXTex/d3dx12.h"
#include <d3d12.h>
#include <wrl.h>

class PostProcess{
public:
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

    void Initialize(DXCommon* dxCommon);
    void Draw(ID3D12GraphicsCommandList* commandList,D3D12_GPU_DESCRIPTOR_HANDLE textureHandle);

private:
    void CreateRootSignature(ID3D12Device* device);
    void CreatePipelineState(ID3D12Device* device,DXCommon* dxCommon);

private:
    ComPtr<ID3D12RootSignature> rootSignature_;
    ComPtr<ID3D12PipelineState> pipelineState_;
};