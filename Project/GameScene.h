#pragma once

#include "BaseScene.h"
#include <memory> // std::unique_ptr用
#include <string> // std::string用

// --- 前方宣言 ---
// ヘッダー内ではポインタとしてしか扱わないクラスは、
// インクルードせずに宣言のみ行うことで依存関係を減らします。
class Input;
class Obj3D;
class Obj3dCommon;
class ParticleEmitter;
class SpriteCommon;

/// <summary>
/// ゲームプレイ画面シーン
/// メインのゲームロジック、3Dオブジェクト配置、パーティクルなどを管理します。
/// </summary>
class GameScene : public BaseScene{
public:
	// --- コンストラクタ・デストラクタ ---
	GameScene();
	~GameScene() override;

	// --- BaseScene オーバーライド ---
	void Initialize(Obj3dCommon* object3dCommon,Input* input,SpriteCommon* spriteCommon) override;
	void Finalize() override;
	void Update() override;
	void Draw() override;

private:
	// --- メンバ変数：外部依存 (借りてくるもの) ---
	// ※Framework側で管理されているため、ここでは参照用の生ポインタとして保持します
	Obj3dCommon* object3dCommon_ = nullptr;
	Input* input_ = nullptr;
	SpriteCommon* spriteCommon_ = nullptr;

	// --- メンバ変数：内部リソース (このシーンが所有するもの) ---
	// ※シーン破棄時に自動的にメモリ解放されるよう、unique_ptrで管理します
	std::shared_ptr<Obj3D> planeObj_;
	std::shared_ptr<Obj3D> fenceObj_;
	std::shared_ptr<Obj3D> sphereObj_;
	std::unique_ptr<ParticleEmitter> particleEmitter_;

	// --- メンバ変数：パラメータ・フラグ ---
	const std::string kBgmPath_ = "resource/You_and_Me.mp3";
	bool isPaused_ = false;
};