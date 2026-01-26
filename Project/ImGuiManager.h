#pragma once
#include <cstdint>
#include <d3d12.h> 
#include <wrl.h>

class WinAPI;
class DXCommon;
class ImGuiManager{
public:

	static ImGuiManager* GetInstance();

	void Initialize(WinAPI* winApi,DXCommon* dxCommon);
	void Finalize();

	void Begin();
	void End();

	void Draw();

private:

	ImGuiManager() = default;
	~ImGuiManager() = default;
	ImGuiManager(const ImGuiManager&) = delete;
	ImGuiManager& operator=(const ImGuiManager&) = delete;


	DXCommon* dxCommon_ = nullptr;
	ID3D12DescriptorHeap* srvHeap_ = nullptr;
	uint32_t srvIndex_;
};

