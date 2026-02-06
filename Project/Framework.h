#pragma once

// --- 必要なヘッダー類 ---
#include "WinAPI.h"
#include "DXCommon.h"
#include "Input.h"
#include "SrvManager.h"
#include "ImGuiManager.h"
#include "SoundManager.h"
#include "CameraManager.h"
#include "ParticleManager.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "SpriteCommon.h"
#include "Obj3DCommon.h"
#include "BaseScene.h"

/// <summary>
/// ゲームの土台となるクラス (エンジン部分)
/// </summary>
class Framework{
public: // --- 仮想関数 ---

	// 仮想デストラクタ (資料通り default で定義)
	virtual ~Framework() = default;

	// 初期化
	virtual void Initialize();

	// 終了
	virtual void Finalize();

	// 毎フレーム更新
	virtual void Update();

	// 描画 (純粋仮想関数: 継承先での実装を強制する)
	virtual void Draw() = 0;

	// 終了リクエストの確認
	virtual bool IsEndRequest(){ return endRequest_; }

protected: // 継承先(MyGame)でも使えるように protected にする

	// 終了フラグ
	bool endRequest_ = false;

	// --- 基盤システム ---
	WinAPI* winApi_ = nullptr;
	DXCommon* dxCommon_ = nullptr;
	Input* input_ = nullptr;

	// --- 描画共通 ---
	SpriteCommon* spriteCommon_ = nullptr;
	Obj3dCommon* object3dCommon_ = nullptr;

	BaseScene* scene_ = nullptr;
};