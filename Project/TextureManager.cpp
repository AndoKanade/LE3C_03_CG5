#include "TextureManager.h"
#include "DXCommon.h"
#include "StringUtility.h"
#include "SrvManager.h"
#include <algorithm>
#include <memory> 

// 静的メンバ変数の初期化
uint32_t TextureManager::kSRVIndexTop = 1;

// ★削除: ポインタ変数の定義は不要です
// TextureManager* TextureManager::instance = nullptr;

TextureManager* TextureManager::GetInstance(){
	// ★変更: staticローカル変数を使う (Meyers Singleton)
	// 最初の1回だけ生成され、アプリ終了時に自動で破棄されます。
	// これにより new も delete も不要になります！
	static TextureManager instance;
	return &instance;
}

// ★変更: Destroyはもう何もしなくてOKです
void TextureManager::Destroy(){
	// delete instance;  <-- ★削除 (絶対にやってはいけない)

	// static変数はプログラム終了時に自動で消えるので、ここは空でOKです。
	// (Framework::Finalize から呼んでも安全なように、空の関数として残しておきます)
}

// 終了処理 (リソースのクリア)
void TextureManager::Finalize(){
	// 中身の unique_ptr たちはここでリセットして、VRAMなどを解放しておく
	textureDatas_.clear();
}

void TextureManager::Initialize(DXCommon* dxCommon,SrvManager* srvManager){
	this->dxCommon_ = dxCommon;
	this->srvManager_ = srvManager;
	textureDatas_.reserve(SrvManager::kMaxSRVCount);
}

void TextureManager::LoadTexture(const std::string& filePath){

	// 読み込み済みなら早期リターン
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

	// unique_ptr を生成
	auto textureData = std::make_unique<TextureData>();

	// データを書き込む
	textureData->metadata = mipImages.GetMetadata();
	textureData->resource = dxCommon_->CreateTextureResource(textureData->metadata);
	textureData->intermediateResource = dxCommon_->UploadTextureData(textureData->resource,mipImages);

	// SRVインデックス割り当て
	textureData->srvIndex = srvManager_->Allocate();

	// ハンドル取得
	textureData->srvHandleCPU = srvManager_->GetCPUDescriptorHandle(textureData->srvIndex);
	textureData->srvHandleGPU = srvManager_->GetGPUDescriptorHandle(textureData->srvIndex);

	// SRV生成
	srvManager_->CreateSRVTexture2D(
		textureData->srvIndex,
		textureData->resource.Get(),
		textureData->metadata.format,
		UINT(textureData->metadata.mipLevels)
	);

	// マップに所有権を移動
	textureDatas_[filePath] = std::move(textureData);
}

// 1. メタデータの取得
const DirectX::TexMetadata& TextureManager::GetMetaData(const std::string& filePath){
	auto it = textureDatas_.find(filePath);
	if(it != textureDatas_.end()){
		return it->second->metadata;
	}
	assert(0 && "Texture not found.");
	static DirectX::TexMetadata defaultMetadata{};
	return defaultMetadata;
}

// 2. SRVインデックスの取得
uint32_t TextureManager::GetSrvIndex(const std::string& filePath){
	auto it = textureDatas_.find(filePath);
	if(it != textureDatas_.end()){
		return it->second->srvIndex;
	}
	assert(0 && "Texture not found.");
	return 0;
}

// 3. GPUハンドルの取得
D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(const std::string& filePath){
	auto it = textureDatas_.find(filePath);
	if(it != textureDatas_.end()){
		return it->second->srvHandleGPU;
	}
	assert(0 && "Texture not found.");
	return D3D12_GPU_DESCRIPTOR_HANDLE{};
}