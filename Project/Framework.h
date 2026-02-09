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
#include "AbstractSceneFactory.h"

// ★追加: unique_ptr用
#include <memory>

/// <summary>
/// ゲームの土台となるクラス (エンジン部分)
/// </summary>
class Framework{
public: // --- 仮想関数 ---

	// 仮想デストラクタ (default で定義)
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

	// --- 基盤システム (Frameworkが所有権を持つ) ---
	// ★変更: 生ポインタから unique_ptr に変更
	std::unique_ptr<WinAPI> winApi_;
	std::unique_ptr<DXCommon> dxCommon_;
	std::unique_ptr<Input> input_;

	// --- 描画共通 (Frameworkが所有権を持つ) ---
	// ★変更: 生ポインタから unique_ptr に変更
	std::unique_ptr<SpriteCommon> spriteCommon_;
	std::unique_ptr<Obj3dCommon> object3dCommon_;

	// --- シーン関連 ---
	// ★変更: 工場も Framework が所有して管理する
	std::unique_ptr<AbstractSceneFactory> sceneFactory_;

	// ※ BaseScene* scene_; は削除しました！
	// 理由は、SceneManager が unique_ptr でシーンを管理するようになったため、
	// Framework 側で二重管理する必要がなくなったからです。
};