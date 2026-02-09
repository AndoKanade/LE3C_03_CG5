#pragma once
#include "BaseScene.h"
#include "AbstractSceneFactory.h"
#include <string>

// --- 前方宣言 (インクルード循環防止・高速化) ---
class Obj3dCommon;
class SpriteCommon;
class Input;

/// <summary>
/// シーン管理クラス (シングルトン)
/// アプリケーション全体のシーン遷移と実行を管理する
/// </summary>
class SceneManager{
private:
	// -------------------------------------------------
	// シングルトン化: コンストラクタ等を隠蔽
	// -------------------------------------------------
	SceneManager() = default;
	~SceneManager();

public:
	// コピーコンストラクタと代入演算子を無効にする (コピー禁止)
	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;

	// -------------------------------------------------
	// 公開メンバ関数
	// -------------------------------------------------

	// インスタンス取得
	static SceneManager* GetInstance();

	// 終了時にインスタンスを破棄する (Application::Finalize で呼ぶ)
	static void Destroy();

	// 更新 (現在のシーンのUpdateを呼ぶ & シーン切り替え処理)
	void Update();

	// 描画 (現在のシーンのDrawを呼ぶ)
	void Draw();

	// 次のシーンを予約する (実際の切り替えは次のUpdate冒頭)
	void ChangeScene(const std::string& sceneName);

	// -------------------------------------------------
	// セッター (依存性の注入)
	// -------------------------------------------------

	// シーン生成工場をセット
	void SetFactory(AbstractSceneFactory* factory){
		sceneFactory_ = factory;
	}

	// 共通データ (Input等) をセット
	// ※Application初期化時に必ず呼ぶこと
	void SetCommonPtr(Obj3dCommon* common,Input* input,SpriteCommon* spriteCommon){
		object3dCommon_ = common;
		input_ = input;
		spriteCommon_ = spriteCommon;
	}

private:
	// -------------------------------------------------
	// メンバ変数
	// -------------------------------------------------

	// シングルトンインスタンス
	static SceneManager* instance_;

	// --- シーン管理 ---
	AbstractSceneFactory* sceneFactory_ = nullptr; // シーン工場
	BaseScene* scene_ = nullptr;                   // 今実行しているシーン
	std::string nextSceneName_ = "";               // 次に実行する予定のシーン名

	// --- 共通データ (借りてくるもの) ---
	Obj3dCommon* object3dCommon_ = nullptr;
	SpriteCommon* spriteCommon_ = nullptr;
	Input* input_ = nullptr;
};