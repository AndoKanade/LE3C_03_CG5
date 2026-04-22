#pragma once
#include "DXCommon.h"
#include "externals/DirectXTex/d3dx12.h"
#include <d3d12.h>
#include <wrl.h>

struct PostProcessData{
    int32_t kernelSize;
};

class PostProcess{
public:
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

    void Initialize(DXCommon* dxCommon);
    void Draw(ID3D12GraphicsCommandList* commandList,D3D12_GPU_DESCRIPTOR_HANDLE textureHandle);

    void SetKernelSize(int32_t k){ constMap_->kernelSize = k; }

private:
    void CreateRootSignature(ID3D12Device* device);
    void CreatePipelineState(ID3D12Device* device,DXCommon* dxCommon);

private:
    ComPtr<ID3D12RootSignature> rootSignature_;
    ComPtr<ID3D12PipelineState> pipelineState_;

    Microsoft::WRL::ComPtr<ID3D12Resource> constBuff_;
    PostProcessData* constMap_ = nullptr;
};