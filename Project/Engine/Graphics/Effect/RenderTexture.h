#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include "MyMath.h" // 自身のプロジェクトのパスに合わせて調整してください

class RenderTexture{
public:
    void Create(
        Microsoft::WRL::ComPtr<ID3D12Device> device,
        uint32_t width,
        uint32_t height,
        DXGI_FORMAT format,
        const Vector4& clearColor,
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCpu, // SRVのCPUハンドル
        D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGpu  // SRVのGPUハンドル（描画コマンド用）
        );

    // リソース取得用
    ID3D12Resource* GetResource() const{ return resource_.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle() const{ return rtvHandle_; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGpu() const{ return srvHandleGpu_; }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle_;
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCpu_;
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGpu_;
};