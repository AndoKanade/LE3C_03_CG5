#include "TextureManager.h"
#include "DXCommon.h"
#include "StringUtility.h"
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

void TextureManager::Initialize(DXCommon* dxCommon){
	this->dxCommon_ = dxCommon;
	textureDatas_.reserve(DXCommon::kMaxSRVCount);
}

void TextureManager::LoadTexture(const std::string& filePath){

	auto it = std::find_if(
		textureDatas_.begin(),
		textureDatas_.end(),
		[&](const auto& textureData){ return textureData.filePath == filePath; }
	);

	if(it != textureDatas_.end()){
		// すでに読み込み済みなら、何もせず終了
		return;
	}
	assert(textureDatas_.size() + kSRVIndexTop < DXCommon::kMaxSRVCount);

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

	textureDatas_.resize(textureDatas_.size() + 1);
	TextureData& textureData = textureDatas_.back();

	textureData.filePath = filePath;
	textureData.metadata = mipImages.GetMetadata();
	textureData.resource = dxCommon_->CreateTextureResource(textureData.metadata);
	uint32_t srvIndex = static_cast<uint32_t>(textureDatas_.size() - 1) + kSRVIndexTop;

	textureData.srvHandleCPU = dxCommon_->GetSRVCPUDescriptorHandle(srvIndex);
	textureData.srvHandleGPU = dxCommon_->GetSRVGPUDescriptorHandle(srvIndex);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = textureData.metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = UINT(textureData.metadata.mipLevels);

	// 4. 設定をもとにSRVの生成
	dxCommon_->GetDevice()->CreateShaderResourceView(
		textureData.resource.Get(), // ビューの対象となるリソース
		&srvDesc,                   // 設定情報
		textureData.srvHandleCPU    // 書き込み先のハンドル
	);

	textureData.intermediateResource = dxCommon_->UploadTextureData(textureData.resource,mipImages);
}

// 指定したファイルパスのSRVハンドル(GPU)を取得する
D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(const std::string& filePath){
	// 範囲ベースfor文で検索
	for(const auto& texture : textureDatas_){
		if(texture.filePath == filePath){
			return texture.srvHandleGPU;
		}
	}
	// 見つからなかったらエラーログを出して止めるなどの処理推奨
	// とりあえず空のハンドルを返す
	return D3D12_GPU_DESCRIPTOR_HANDLE{};
}

// 指定したファイルパスのメタデータを取得する
const DirectX::TexMetadata& TextureManager::GetMetaData(const std::string& filePath){
	for(const auto& texture : textureDatas_){
		if(texture.filePath == filePath){
			return texture.metadata;
		}
	}
	// 見つからなかった場合（例外処理が必要だが、一旦デフォルト値を返す）
	static DirectX::TexMetadata defaultMetadata{};
	return defaultMetadata;
}

uint32_t TextureManager::GetTextureIndexbyFilePath(const std::string& filePath){
	auto it = std::find_if(
		textureDatas_.begin(),
		textureDatas_.end(),
		[&](const auto& textureData){ return textureData.filePath == filePath; }
	);

	if(it != textureDatas_.end()){
		uint32_t textureIndex = static_cast<uint32_t>(std::distance(textureDatas_.begin(),it));
		return textureIndex;
	}
	assert(0);
	return 0;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(uint32_t textureIndex){
	// 1. 範囲外指定違反チェック
	// 「テクスチャ番号が正常範囲内である」かを確認
	assert(textureIndex < textureDatas_.size());

	// 2. テクスチャデータの参照を取得
	// 配列（またはvector）から直接インデックスでアクセスします
	TextureData& textureData = textureDatas_[textureIndex];

	// 3. 保持しているGPUハンドルを返す
	return textureData.srvHandleGPU;
}
