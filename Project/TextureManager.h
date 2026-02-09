#pragma once
#include <string>
#include <vector> 
#include <unordered_map> 
#include <DirectXTex.h>
#include <wrl.h>
#include <d3d12.h>
#include <memory> // ★追加: unique_ptr用

class DXCommon;
class SrvManager;

class TextureManager{
private: // ▼ 自分だけが知っていればいい情報

	// シングルトン管理用
	static TextureManager* instance;
	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(TextureManager&) = delete;
	TextureManager& operator=(TextureManager&) = delete;

	// テクスチャ1枚分のデータ構造体
	struct TextureData{
		std::string filePath;
		DirectX::TexMetadata metadata;
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		uint32_t srvIndex;
		Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
	};

	// テクスチャデータ一覧
	// ★変更: 中身を unique_ptr にすることで、マップのリサイズ時に重いコピーが発生しなくなります
	std::unordered_map<std::string,std::unique_ptr<TextureData>> textureDatas_;

	static uint32_t kSRVIndexTop;

	// DXCommonのポインタ
	DXCommon* dxCommon_ = nullptr;
	// SrvManagerのポインタ
	SrvManager* srvManager_ = nullptr;

public: // ▼ 外部から呼び出したい機能

	// シングルトン取得
	static TextureManager* GetInstance();

	// ★追加: シングルトン破棄 (Framework::Finalize で呼ぶ)
	static void Destroy();

	// 初期化・終了
	void Initialize(DXCommon* dxCommon,SrvManager* srvManager);
	void Finalize();

	// テクスチャ読み込み
	void LoadTexture(const std::string& filePath);

	// テクスチャのGPUハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(const std::string& filePath);

	uint32_t GetSrvIndex(const std::string& filePath);

	// メタデータ取得
	const DirectX::TexMetadata& GetMetaData(const std::string& filePath);
};