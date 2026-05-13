#pragma once
#include "DXCommon.h"
#include <d3d12.h>
#include <wrl.h>
#include <map>
#include <string>

class PostProcess{
public:
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

	// --- 列挙型定義 ---
	enum class Type{
		PostProcess, // 標準（パススルー）
		BoxFilter,   // ボックスフィルタ（ブラー）
		Grayscale,   // グレースケール
		Vignette,    // ビネット（周辺減光）
		GaussianBlur,  // ガウシアンブラー
		Outline,      // アウトライン
	};

	// --- 定数バッファ構造体 (16byteアライメント) ---
	struct PostProcessData{
		int32_t kernelSize;       // BoxFilter用
		float vignetteIntensity;  // Vignette用：強さ
		float vignetteScale;      // Vignette用：範囲
		float padding;            // 16byte境界に合わせるためのパディング
	};

public:
	// --- 基本メンバ関数 ---
	void Initialize(DXCommon* dxCommon);
	void Draw(ID3D12GraphicsCommandList* commandList,D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,D3D12_GPU_DESCRIPTOR_HANDLE depthTextureHandle,Type type);

	// --- アクセサ (Set関数) ---
	void SetKernelSize(int32_t k){ if(constMap_) constMap_->kernelSize = k; }
	void SetVignetteIntensity(float intensity){ if(constMap_) constMap_->vignetteIntensity = intensity; }
	void SetVignetteScale(float scale){ if(constMap_) constMap_->vignetteScale = scale; }

private:
	// --- 内部処理関数 ---
	void CreateRootSignature(ID3D12Device* device);
	void CreatePipelineState(ID3D12Device* device,DXCommon* dxCommon,Type type,const std::wstring& filename);

private:
	// --- メンバリソース ---
	ComPtr<ID3D12RootSignature> rootSignature_;

	// TypeごとのPipelineStateを管理するマップ
	std::map<Type,ComPtr<ID3D12PipelineState>> pipelineStates_;

	// 定数バッファ
	ComPtr<ID3D12Resource> constBuff_;
	PostProcessData* constMap_ = nullptr;
};