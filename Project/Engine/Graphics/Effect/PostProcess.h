#pragma once
#include "DXCommon.h"
#include <d3d12.h>
#include <wrl.h>
#include <map>
#include <string>

class PostProcess{
public:
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

    // ポストプロセスの種類
    enum class Type{
        PostProcess,
        BoxFilter,
        Grayscale,
        Vignette,
        GaussianBlur,
        LuminanceOutline,
        DepthOutline,
    };

    // 定数バッファ構造体
    struct PostProcessData{
        int32_t kernelSize;
        float vignetteIntensity;
        float vignetteScale;
        float padding;
    };

public:
    void Initialize(DXCommon* dxCommon);
    void Draw(ID3D12GraphicsCommandList* commandList,D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,D3D12_GPU_DESCRIPTOR_HANDLE depthTextureHandle,Type type);

    // パラメータ設定
    void SetKernelSize(int32_t k){ if(constMap_) constMap_->kernelSize = k; }
    void SetVignetteIntensity(float intensity){ if(constMap_) constMap_->vignetteIntensity = intensity; }
    void SetVignetteScale(float scale){ if(constMap_) constMap_->vignetteScale = scale; }

private:
    void CreateRootSignature(ID3D12Device* device);
    void CreatePipelineState(ID3D12Device* device,DXCommon* dxCommon,Type type,const std::wstring& filename);

private:
    ComPtr<ID3D12RootSignature> rootSignature_;
    std::map<Type,ComPtr<ID3D12PipelineState>> pipelineStates_;
    ComPtr<ID3D12Resource> constBuff_;
    PostProcessData* constMap_ = nullptr;
};