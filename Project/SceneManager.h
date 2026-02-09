#pragma once
#include "BaseScene.h"
#include "AbstractSceneFactory.h"
#include <string>
#include <memory> 

// --- 前方宣言 ---
class Obj3dCommon;
class SpriteCommon;
class Input;

/// <summary>
/// シーン管理クラス (シングルトン)
/// </summary>
class SceneManager{
private:
	// コンストラクタ隠蔽
	SceneManager() = default;
	~SceneManager() = default;

public:
	// コピー禁止
	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;

	// -------------------------------------------------
	// 公開メンバ関数
	// -------------------------------------------------

	// インスタンス取得 (Meyers Singleton)
	static SceneManager* GetInstance();

	// ★変更: static Destroy() を廃止し、リソース解放用のメンバ関数にする
	void Finalize();

	// 更新
	void Update();

	// 描画
	void Draw();

	// 次のシーン予約
	void ChangeScene(const std::string& sceneName);

	// -------------------------------------------------
	// セッター
	// -------------------------------------------------
	void SetFactory(AbstractSceneFactory* factory){
		sceneFactory_ = factory;
	}

	void SetCommonPtr(Obj3dCommon* common,Input* input,SpriteCommon* spriteCommon){
		object3dCommon_ = common;
		input_ = input;
		spriteCommon_ = spriteCommon;
	}

private:
	// -------------------------------------------------
	// メンバ変数
	// -------------------------------------------------

	// ★削除: static SceneManager* instance_; は不要になりました

	// シーン管理
	AbstractSceneFactory* sceneFactory_ = nullptr;

	// 現在のシーン (unique_ptr なので自動解放されます)
	std::unique_ptr<BaseScene> scene_;

	std::string nextSceneName_ = "";

	// 共通データ
	Obj3dCommon* object3dCommon_ = nullptr;
	SpriteCommon* spriteCommon_ = nullptr;
	Input* input_ = nullptr;
};