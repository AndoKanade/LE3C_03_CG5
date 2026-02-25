#pragma once

#include "BaseScene.h"
#include"Math.h"
#include <memory> // unique_ptr用
#include <string> // 必要に応じて

// --- 前方宣言 ---
// ヘッダー内ではポインタとしてしか扱わないため、includeではなくclass宣言のみで済ませる
// これによりコンパイル時間を短縮し、循環参照を防ぎます
class Input;
class Obj3D;
class Obj3dCommon;
class Sprite;
class SpriteCommon;

/// <summary>
/// タイトル画面シーン
/// </summary>
class TitleScene : public BaseScene{
public:
	// --- コンストラクタ・デストラクタ ---
	TitleScene();
	~TitleScene() override;

	// --- BaseScene オーバーライド ---
	void Initialize(Obj3dCommon* object3dCommon,Input* input,SpriteCommon* spriteCommon) override;
	void Finalize() override;
	void Update() override;
	void Draw() override;

private:
	// --- メンバ変数：外部依存 (借りてくるもの) ---
	// ※これらはMain側(SceneManager等)が寿命を管理しているため、ここでは生ポインタで参照のみ保持する
	Obj3dCommon* object3dCommon_ = nullptr;
	Input* input_ = nullptr;
	SpriteCommon* spriteCommon_ = nullptr;

	// --- メンバ変数：内部リソース (所有するもの) ---
	// ※このシーンが生成・管理し、シーン破棄とともに消えるものは unique_ptr で管理する
	std::unique_ptr<Obj3D> titleObject_;
	std::unique_ptr<Sprite> sprite_;

	// --- メンバ変数：パラメータ ---
	Vector4 spriteColor_ = {1.0f, 1.0f, 1.0f, 1.0f};
};