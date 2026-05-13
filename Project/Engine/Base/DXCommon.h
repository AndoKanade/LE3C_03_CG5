#pragma once
#include "WinAPI.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <DirectXTex.h>
#include <wrl.h>
#include <array>
#include <chrono>
#include <string>
#include <cstdint>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

// 前方宣言
class RenderTexture;

class DXCommon{
public:
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public: // --- メンバリソース ---

    // Direct3D 12 Core
    ComPtr<IDXGIFactory7> dxgiFactory;
    ComPtr<ID3D12Device> device;

    // Command Objects
    ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
    ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;
    ComPtr<ID3D12CommandQueue> commandQueue = nullptr;

    // SwapChain & BackBuffers
    ComPtr<IDXGISwapChain4> swapChain = nullptr;
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    std::array<ComPtr<ID3D12Resource>,2> swapChainResources;

    // RTV (Render Target View)
    ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap = nullptr;
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
    uint32_t descriptorSizeRTV = 0;

    // DSV (Depth Stencil View)
    ComPtr<ID3D12Resource> depthStencilResource = nullptr;
    ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;
    uint32_t descriptorSizeDSV = 0;

    // SRV (Shader Resource View) for PostProcess
    D3D12_GPU_DESCRIPTOR_HANDLE depthSrvHandleGpu_;

    // Synchronization
    ComPtr<ID3D12Fence> fence = nullptr;
    uint64_t fenceValue = 0;
    HANDLE fenceEvent = nullptr;
    D3D12_RESOURCE_BARRIER barrier{};

    // Rendering States
    D3D12_VIEWPORT viewport{};
    D3D12_RECT scissorRect{};

    // DXC (Shader Compiler)
    ComPtr<IDxcUtils> dxcUtils = nullptr;
    ComPtr<IDxcCompiler3> dxcCompiler = nullptr;
    IDxcIncludeHandler* includeHandler = nullptr;

public: // --- 初期化・描画フロー ---

    void Initialize(WinAPI* winApi);

    // 初期化ステップ
    void InitDevice();
    void InitCommand();
    void CreateSwapChain();
    void CreateDepthBuffer();
    void CreateDescriptorHeaps();
    void InitRenderTargetView();
    void InitDepthStancilView();
    void InitDepthShaderResourceView();
    void InitFence();
    void InitViewportRect();
    void InitScissorRect();
    void CreateDXCCompiler();

    // フレーム描画
    void PreDraw(RenderTexture* renderTexture = nullptr);
    void PostDraw();

public: // --- ユーティリティ・リソース生成 ---

    ComPtr<ID3D12DescriptorHeap> CreateDiscriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType,UINT numDescriptors,bool shaderVisible);
    ComPtr<IDxcBlob> CompileShader(const std::wstring& filePath,const wchar_t* profile);
    ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);
    ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);
    ComPtr<ID3D12Resource> UploadTextureData(const ComPtr<ID3D12Resource>& texture,const DirectX::ScratchImage& mipImages);

public: // --- アクセッサ ---

    ID3D12Device* GetDevice() const{ return device.Get(); }
    ID3D12GraphicsCommandList* GetCommandList() const{ return commandList.Get(); }
    ID3D12CommandQueue* GetCommandQueue() const{ return commandQueue.Get(); }
    size_t GetSwapChainResourcesNum() const{ return swapChainResources.size(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle() const{ return dsvHandle; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetDepthSrvHandleGpu() const{ return depthSrvHandleGpu_; }

    void SetDepthSrvHandleGpu(D3D12_GPU_DESCRIPTOR_HANDLE handle){ depthSrvHandleGpu_ = handle; }

private: // --- 内部処理・FPS制御 ---

    WinAPI* winApi_ = nullptr;
    std::chrono::steady_clock::time_point reference_;

    void InitializeFixFPS();
    void UpdateFixFPS();

    // 静的ヘルパー関数
    static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const ComPtr<ID3D12DescriptorHeap>& descriptorHeap,uint32_t descriptorSize,uint32_t index){
        D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
        handleCPU.ptr += (size_t)descriptorSize * index;
        return handleCPU;
    }

    static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDscriptorHandle(const ComPtr<ID3D12DescriptorHeap>& descriptorHeap,uint32_t descriptorSize,uint32_t index){
        D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
        handleGPU.ptr += (size_t)descriptorSize * index;
        return handleGPU;
    }
};