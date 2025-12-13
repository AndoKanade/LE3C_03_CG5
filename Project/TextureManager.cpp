#include "TextureManager.h"
#include "DXCommon.h"
#include "StringUtility.h"
#include"SrvManager.h"
#include <algorithm>

uint32_t TextureManager::kSRVIndexTop = 1;
TextureManager* TextureManager::instance = nullptr;
TextureManager* TextureManager::GetInstance(){
	if(instance == nullptr){
		instance = new TextureManager;
	}

	return instance;
}

void TextureManager::Finalize(){
	delete instance;
	instance = nullptr;

}

void TextureManager::Initialize(DXCommon* dxCommon,SrvManager* srvManager){
	this->dxCommon_ = dxCommon;
	this->srvManager_ = srvManager;
	textureDatas_.reserve(SrvManager::kMaxSRVCount);
}

void TextureManager::LoadTexture(const std::string& filePath){

	// 読み込み済みなら早期リターン
	// (C++20なら contains が使えますが、汎用的に find を使っています)
	if(textureDatas_.contains(filePath)){
		return;
	}
	assert(srvManager_->CanAllocate());

	// --- 画像の読み込み処理 ---
	DirectX::ScratchImage image{};
	std::wstring filePathW = StringUtility::ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(
		filePathW.c_str(),DirectX::WIC_FLAGS_FORCE_SRGB,nullptr,image);
	assert(SUCCEEDED(hr));

	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(),image.GetImageCount(),
		image.GetMetadata(),DirectX::TEX_FILTER_SRGB,
		0,mipImages);
	assert(SUCCEEDED(hr));
	// -----------------------

	// 【変更点1】マップのキーを指定して参照を取得（ここで要素が追加されます）
	TextureData& textureData = textureDatas_[filePath];

	// データを書き込む
	textureData.metadata = mipImages.GetMetadata();
	textureData.resource = dxCommon_->CreateTextureResource(textureData.metadata);
	textureData.intermediateResource = dxCommon_->UploadTextureData(textureData.resource,mipImages);

	// 【変更点2】SRVマネージャからインデックスを割り当ててもらう (Allocate)
	// 以前のような計算 (size + offset) は不要になります
	textureData.srvIndex = srvManager_->Allocate();

	// 【変更点3】割り当てられたインデックスを使ってハンドルを取得
	textureData.srvHandleCPU = srvManager_->GetCPUDescriptorHandle(textureData.srvIndex);
	textureData.srvHandleGPU = srvManager_->GetGPUDescriptorHandle(textureData.srvIndex);

	// SRV生成 (前回作成した関数を使用)
	srvManager_->CreateSRVTexture2D(
		textureData.srvIndex,
		textureData.resource.Get(),
		textureData.metadata.format,
		UINT(textureData.metadata.mipLevels)
	);
}

// 1. メタデータの取得
const DirectX::TexMetadata& TextureManager::GetMetaData(const std::string& filePath){
	// マップから検索
	auto it = textureDatas_.find(filePath);

	// 見つかった場合
	if(it != textureDatas_.end()){
		return it->second.metadata;
	}

	// 見つからなかった場合のエラー処理
	assert(0 && "Texture not found.");
	static DirectX::TexMetadata defaultMetadata{};
	return defaultMetadata;
}

// 2. SRVインデックスの取得
uint32_t TextureManager::GetSrvIndex(const std::string& filePath){
	// マップから検索
	auto it = textureDatas_.find(filePath);

	// 見つかった場合
	if(it != textureDatas_.end()){
		return it->second.srvIndex;
	}

	// 見つからなかった場合のエラー処理
	assert(0 && "Texture not found.");
	return 0;
}

// 3. GPUハンドルの取得
D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(const std::string& filePath){
	// マップから検索
	auto it = textureDatas_.find(filePath);

	// 見つかった場合
	if(it != textureDatas_.end()){
		return it->second.srvHandleGPU;
	}

	// 見つからなかった場合のエラー処理
	assert(0 && "Texture not found.");
	return D3D12_GPU_DESCRIPTOR_HANDLE{};
}