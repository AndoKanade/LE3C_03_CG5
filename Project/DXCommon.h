#pragma once
#include "WinAPI.h"
#include <DirectXTex.h>
#include <array>
#include <d3d12.h>
#include<chrono>

#include <dxcapi.h>
#include <dxgi1_6.h>
#include <string>
#include <wrl.h>

class DXCommon{
public:
#pragma region メンバ変数
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
	Microsoft::WRL::ComPtr<ID3D12Device> device;

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = nullptr;

	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,2> swapChainResources;

	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;

	uint32_t descriptorSizeSRV = 0;
	uint32_t descriptorSizeRTV = 0;
	uint32_t descriptorSizeDSV = 0;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap = nullptr;

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,2> backBuffers;

	Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;

	D3D12_VIEWPORT viewport{};
	D3D12_RECT scissorRect{};

	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils = nullptr;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler = nullptr;
	IDxcIncludeHandler* includeHandler = nullptr;

	D3D12_RESOURCE_BARRIER barrier{};
	uint64_t fenceValue = 0;
	HANDLE fenceEvent = nullptr;

	static const uint32_t kMaxSRVCount;

#pragma endregion

#pragma region メンバ関数

	void Initialize(WinAPI* winApi);

	void InitDevice();
	void InitCommand();
	void CreateSwapChain();
	void CreateDepthBuffer();
	void CreateDescriptorHeaps();
	void InitRenderTargetView();
	void InitDepthStancilView();
	void InitFence();
	void InitViewportRect();
	void InitScissorRect();
	void CreateDXCCompiler();
	void InitImGui();

	void PreDraw();
	void PostDraw();

#pragma endregion

#pragma region ユーティリティ関数
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>
		CreateDiscriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType,UINT numDescriptors,
			bool shaderVisible);

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

	Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(const std::wstring& filePath,
		const wchar_t* profile);

	Microsoft::WRL::ComPtr<ID3D12Resource>
		CreateBufferResource(size_t sizeInBytes);

	Microsoft::WRL::ComPtr<ID3D12Resource>
		CreateTextureResource(const DirectX::TexMetadata& metadata);

	Microsoft::WRL::ComPtr<ID3D12Resource>
		UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture,
			const DirectX::ScratchImage& mipImages);

#pragma endregion

	ID3D12Device* GetDevice() const{ return device.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() const{
		return commandList.Get();
	}

private:
	WinAPI* winApi_ = nullptr;

	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(
		const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
		uint32_t descriptorSize,uint32_t index){
		D3D12_CPU_DESCRIPTOR_HANDLE handleCPU =
			descriptorHeap->GetCPUDescriptorHandleForHeapStart();
		handleCPU.ptr += descriptorSize * index;
		return handleCPU;
	}

	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDscriptorHandle(
		const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
		uint32_t descriptorSize,uint32_t index){
		D3D12_GPU_DESCRIPTOR_HANDLE handleGPU =
			descriptorHeap->GetGPUDescriptorHandleForHeapStart();
		handleGPU.ptr += descriptorSize * index;
		return handleGPU;
	}

	void InitializeFixFPS();
	void UpdateFixFPS();

	std::chrono::steady_clock::time_point reference_;
};