#pragma once
#include "BaseScene.h"

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
	void ChangeScene(BaseScene* nextScene);

private:
	// -------------------------------------------------
	// メンバ変数
	// -------------------------------------------------

	// シングルトンインスタンス
	static SceneManager* instance_;

	// 今実行しているシーン
	BaseScene* scene_ = nullptr;

	// 次に実行する予定のシーン (予約用)
	BaseScene* nextScene_ = nullptr;
};