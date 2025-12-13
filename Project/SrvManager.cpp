#include "SrvManager.h"
#include "DXCommon.h"
#include <cassert>

// SRVの最大数
const uint32_t SrvManager::kMaxSRVCount = 512;

// シングルトン / システム
SrvManager* SrvManager::GetInstance(){
	static SrvManager instance;
	return &instance;
}

void SrvManager::Initialize(DXCommon* dxCommon){
	assert(dxCommon);
	this->dxCommon_ = dxCommon;

	// SRVヒープの生成 (DXCommonの関数を利用)
	// ※CreateDiscriptorHeapのスペルはDXCommon側に合わせる
	descriptorHeap = dxCommon_->CreateDiscriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,kMaxSRVCount,true);

	// デスクリプタ1個分のサイズを取得
	descriptorSize = dxCommon_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// インデックス初期化
	useIndex = 0;
}

void SrvManager::Finalize(){
	// ディスクリプタヒープを解放
	descriptorHeap.Reset();

	// 使用インデックスをリセット
	useIndex = 0;

	dxCommon_ = nullptr;
}

// 描画前処理
void SrvManager::PreDraw(){
	// SRV用ヒープをコマンドリストにセット
	ID3D12DescriptorHeap* descriptorHeaps[] = {descriptorHeap.Get()};
	dxCommon_->GetCommandList()->SetDescriptorHeaps(1,descriptorHeaps);
}

// デスクリプタ管理
uint32_t SrvManager::Allocate(){
	// 上限チェック
	assert(CanAllocate());

	// 現在の番号を確保
	int index = useIndex;

	// 次回のために進める
	useIndex++;

	return index;
}

bool SrvManager::CanAllocate() const{
	return useIndex < kMaxSRVCount;
}

// ハンドル取得

D3D12_CPU_DESCRIPTOR_HANDLE SrvManager::GetCPUDescriptorHandle(uint32_t index){
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE SrvManager::GetGPUDescriptorHandle(uint32_t index){
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}

// SRV生成・設定

void SrvManager::SetGraphicsRootDescriptorTable(UINT RootParameterIndex,uint32_t srvIndex){
	dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(RootParameterIndex,GetGPUDescriptorHandle(srvIndex));
}

void SrvManager::CreateSRVTexture2D(uint32_t srvIndex,ID3D12Resource* pResource,DXGI_FORMAT Format,UINT MipLevels){
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = MipLevels;

	dxCommon_->GetDevice()->CreateShaderResourceView(pResource,&srvDesc,GetCPUDescriptorHandle(srvIndex));
}

void SrvManager::CreateSRVforStructuredBuffer(uint32_t srvIndex,ID3D12Resource* pResource,UINT numElements,UINT structureByteStride){
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN; // StructuredBufferはUNKNOWNでOK
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = numElements;
	srvDesc.Buffer.StructureByteStride = structureByteStride;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	dxCommon_->GetDevice()->CreateShaderResourceView(pResource,&srvDesc,GetCPUDescriptorHandle(srvIndex));
}