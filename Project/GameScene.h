#pragma once

// --- インクルード ---
#include "BaseScene.h"
#include "Obj3D.h"
#include "ParticleEmitter.h"
#include "Obj3DCommon.h"
#include "Input.h"
#include <string> // std::string用
#include <memory> // ★追加: unique_ptr用

/// <summary>
/// ゲームプレイ画面のシーン
/// </summary>

// 前方宣言
class SpriteCommon;

class GameScene : public BaseScene{
public:
	// コンストラクタ・デストラクタ
	// ★変更: ヘッダーでは「宣言（;）」だけにする！
	// （実装は .cpp に書くことで、不完全型エラーやリンクエラーを防ぎます）
	GameScene();
	~GameScene() override;

	// --- BaseSceneの純粋仮想関数をオーバーライド ---
	void Initialize(Obj3dCommon* object3dCommon,Input* input,SpriteCommon* spriteCommon) override;
	void Finalize() override;
	void Update() override;
	void Draw() override;

private:
	// --- システムポインタ (借りてくるもの) ---
	// ※これらは生ポインタのままでOK
	Obj3dCommon* object3dCommon_ = nullptr;
	Input* input_ = nullptr;
	SpriteCommon* spriteCommon_ = nullptr;

	// --- ゲームオブジェクト (このシーンが所有するもの) ---
	// ★変更: 所有権を持つ変数は unique_ptr にする
	std::unique_ptr<Obj3D> planeObj_;
	std::unique_ptr<Obj3D> fenceObj_;
	std::unique_ptr<ParticleEmitter> particleEmitter_;

	// --- 設定・リソース・フラグ ---
	const std::string kBgmPath_ = "resource/You_and_Me.mp3";
	bool isPaused_ = false;
};