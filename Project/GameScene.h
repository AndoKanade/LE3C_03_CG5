#pragma once

// --- インクルード ---
#include "BaseScene.h"
#include "Obj3D.h"
#include "ParticleEmitter.h"
#include "Obj3DCommon.h"
#include "Input.h"
#include <string> // std::string用

/// <summary>
/// ゲームプレイ画面のシーン
/// </summary>
 
class SpriteCommon;

class GameScene : public BaseScene{
public:
	// コンストラクタ・デストラクタ
	GameScene() = default;
	~GameScene() override;

	// --- BaseSceneの純粋仮想関数をオーバーライド ---
	void Initialize(Obj3dCommon* object3dCommon,Input* input,SpriteCommon* spriteCommon) override;	void Finalize() override;
	void Update() override;
	void Draw() override;

private:
	// --- システムポインタ (借りてくるもの) ---
	Obj3dCommon* object3dCommon_ = nullptr;
	Input* input_ = nullptr;
	SpriteCommon* spriteCommon_ = nullptr;

	// --- ゲームオブジェクト (このシーンが所有するもの) ---
	Obj3D* planeObj_ = nullptr;
	Obj3D* fenceObj_ = nullptr;
	ParticleEmitter* particleEmitter_ = nullptr;

	// --- 設定・リソース・フラグ ---
	const std::string kBgmPath_ = "resource/You_and_Me.mp3";
	bool isPaused_ = false;
};