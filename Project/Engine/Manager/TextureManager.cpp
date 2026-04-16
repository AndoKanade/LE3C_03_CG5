#include "TextureManager.h"
#include "DXCommon.h"
#include "StringUtility.h"
#include "SrvManager.h"
#include <algorithm>
#include <memory>

// 静的メンバ変数の初期化
uint32_t TextureManager::kSRVIndexTop = 1;

// インスタンス取得 (Meyers Singleton)
TextureManager* TextureManager::GetInstance(){
	static TextureManager instance;
	return &instance;
}

// 初期化処理
void TextureManager::Initialize(DXCommon* dxCommon,SrvManager* srvManager){
	this->dxCommon_ = dxCommon;
	this->srvManager_ = srvManager;
	textureDatas_.reserve(SrvManager::kMaxSRVCount);
}

// 終了処理
void TextureManager::Finalize(){
	textureDatas_.clear();
}

// テクスチャの読み込み
void TextureManager::LoadTexture(const std::string& filePath){
	// 読み込み済みなら早期リターン
	if(textureDatas_.contains(filePath)){
		return;
	}
	assert(srvManager_->CanAllocate());

	// --- 画像の読み込み処理 ---
	DirectX::ScratchImage image{};
	std::wstring filePathW = StringUtility::ConvertString(filePath);
	HRESULT hr;

	if(filePathW.ends_with(L".dds")){
		hr = DirectX::LoadFromDDSFile(filePathW.c_str(),DirectX::DDS_FLAGS_NONE,nullptr,image);
	} else{
		hr = DirectX::LoadFromWICFile(filePathW.c_str(),DirectX::WIC_FLAGS_FORCE_SRGB,nullptr,image);
	}
	assert(SUCCEEDED(hr));

	// --- ミップマップ生成 ---
	DirectX::ScratchImage mipImages{};
	if(DirectX::IsCompressed(image.GetMetadata().format)){
		mipImages = std::move(image);
	} else{
		hr = DirectX::GenerateMipMaps(
			image.GetImages(),image.GetImageCount(),
			image.GetMetadata(),DirectX::TEX_FILTER_SRGB,
			0,mipImages
		);
	}
	assert(SUCCEEDED(hr));

	// --- リソースの生成と登録 ---
	auto textureData = std::make_unique<TextureData>();

	textureData->metadata = mipImages.GetMetadata();
	textureData->resource = dxCommon_->CreateTextureResource(textureData->metadata);
	textureData->intermediateResource = dxCommon_->UploadTextureData(textureData->resource,mipImages);

	// SRV関連の割り当て
	textureData->srvIndex = srvManager_->Allocate();
	textureData->srvHandleCPU = srvManager_->GetCPUDescriptorHandle(textureData->srvIndex);
	textureData->srvHandleGPU = srvManager_->GetGPUDescriptorHandle(textureData->srvIndex);

	// SRV生成
	srvManager_->CreateSRVTexture(
		textureData->srvIndex,
		textureData->resource.Get(),
		textureData->metadata.format,
		UINT(textureData->metadata.mipLevels)
	);

	// 管理マップに追加
	textureDatas_[filePath] = std::move(textureData);
}

// メタデータの取得
const DirectX::TexMetadata& TextureManager::GetMetaData(const std::string& filePath){
	auto it = textureDatas_.find(filePath);
	if(it != textureDatas_.end()){
		return it->second->metadata;
	}
	assert(0 && "Texture not found.");
	static DirectX::TexMetadata defaultMetadata{};
	return defaultMetadata;
}

// SRVインデックスの取得
uint32_t TextureManager::GetSrvIndex(const std::string& filePath){
	auto it = textureDatas_.find(filePath);
	if(it != textureDatas_.end()){
		return it->second->srvIndex;
	}
	assert(0 && "Texture not found.");
	return 0;
}

// GPUハンドルの取得
D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(const std::string& filePath){
	auto it = textureDatas_.find(filePath);
	if(it != textureDatas_.end()){
		return it->second->srvHandleGPU;
	}
	assert(0 && "Texture not found.");
	return D3D12_GPU_DESCRIPTOR_HANDLE{};
}

// 解放処理 (空の関数として維持)
void TextureManager::Destroy(){}