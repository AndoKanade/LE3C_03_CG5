#pragma once
#include "DXCommon.h"
#include <d3d12.h>
#include <wrl.h>
#include "Camera.h"
#include "Obj3D.h"

struct CameraForGPU{
	Vector3 worldPosition;
};

class Obj3dCommon{

public: // 外部から呼び出すもの

	// 初期化
	void Initialize(DXCommon* dxCommon);

	// 描画設定（コマンドリストへのセットなど）
	void Draw();

	void SetDefaultCamera(Camera* camera){ this->defaultCamera = camera; }


	// ゲッター（読み取り専用）
	// const をつけることで、取得したポインタの中身を書き換えられないことを保証できる場合もありますが、
	// DirectXのポインタはそのまま渡すのが一般的なのでこのままでOKです。
	ID3D12RootSignature* GetRootSignature() const{ return rootSignature.Get(); }
	ID3D12PipelineState* GetGraphicsPipelineState() const{ return graphicsPipelineState.Get(); }
	DXCommon* GetDxCommon() const{ return dxCommon_; }
	Camera* GetDefaultCamera() const{ return defaultCamera; }


private: // 内部でしか使わない関数
	void CreateRootSignature();
	void CreateGraphicsPipelineState();

private: // 内部データ（外部から勝手に書き換えられたくない）

	DXCommon* dxCommon_ = nullptr;
	Camera* defaultCamera = nullptr;
	// 最終的な成果物だけを持っておく
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;

	Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;
	CameraForGPU* cameraData_ = nullptr;
	Camera* defaultCamera_ = nullptr; // 現在のカメラへのポインタ
};