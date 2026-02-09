#pragma once
#include "BaseScene.h"
#include "Obj3D.h"
#include "Obj3DCommon.h"
#include "Input.h"

/// <summary>
/// タイトル画面シーン
/// </summary>
/// 

class SpriteCommon;
class Sprite;

class TitleScene : public BaseScene{
public:
	// コンストラクタ・デストラクタ
	TitleScene() = default;
	~TitleScene() override;

	// --- BaseSceneの純粋仮想関数をオーバーライド ---
	void Initialize(Obj3dCommon* object3dCommon,Input* input,SpriteCommon* spriteCommon) override;	
	void Finalize() override;
	void Update() override;
	void Draw() override;

private:
	// --- システムポインタ (借りてくるもの) ---
	Obj3dCommon* object3dCommon_ = nullptr;
	Input* input_ = nullptr;

	// --- ゲームオブジェクト (このシーンが所有するもの) ---
	Obj3D* titleObject_ = nullptr;

	Sprite* sprite_ = nullptr; // 表示するスプライト
	SpriteCommon* spriteCommon_ = nullptr; // 保存用
};