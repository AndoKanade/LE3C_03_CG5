#pragma once
#include "BaseScene.h"
#include "Obj3D.h"
#include "Obj3DCommon.h"
#include "Input.h"

// ★追加：スマートポインタを使うために必須
#include <memory>

/// <summary>
/// タイトル画面シーン
/// </summary>
/// 

class SpriteCommon;
class Sprite;

class TitleScene : public BaseScene{
public:
	// コンストラクタ・デストラクタ
	TitleScene();
	~TitleScene() override;

	// --- BaseSceneの純粋仮想関数をオーバーライド ---
	void Initialize(Obj3dCommon* object3dCommon,Input* input,SpriteCommon* spriteCommon) override;
	void Finalize() override;
	void Update() override;
	void Draw() override;

private:
	// --- システムポインタ (借りてくるもの) ---
	// ※これらはMain側が持っているので、勝手にdeleteしないよう「生ポインタ」のままにする
	Obj3dCommon* object3dCommon_ = nullptr;
	Input* input_ = nullptr;
	SpriteCommon* spriteCommon_ = nullptr; // 保存用

	// --- ゲームオブジェクト (このシーンが所有するもの) ---
	// ★変更：所有権を持つものは unique_ptr にする！
	// Obj3D* titleObject_ = nullptr;
	std::unique_ptr<Obj3D> titleObject_;

	// Sprite* sprite_ = nullptr; 
	std::unique_ptr<Sprite> sprite_; // 表示するスプライト

	Vector4 spriteColor_ = {1.0f, 1.0f, 1.0f, 1.0f};
};