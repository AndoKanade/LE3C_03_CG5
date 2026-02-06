#include "SceneManager.h"

// --- 静的メンバ変数の実体定義 ---
SceneManager* SceneManager::instance_ = nullptr;

// -------------------------------------------------
// シングルトン管理
// -------------------------------------------------

// インスタンス取得
SceneManager* SceneManager::GetInstance(){
	if(instance_ == nullptr){
		instance_ = new SceneManager();
	}
	return instance_;
}

// 終了処理 (Application::Finalize で呼ばれる)
void SceneManager::Destroy(){
	if(instance_ != nullptr){
		delete instance_;
		instance_ = nullptr;
	}
}

// -------------------------------------------------
// コンストラクタ・デストラクタ
// -------------------------------------------------

// デストラクタ
SceneManager::~SceneManager(){
	// 実行中のシーンがあれば終了処理
	if(scene_){
		scene_->Finalize();
		delete scene_;
		scene_ = nullptr;
	}

	// 予約中のシーンがあれば解放 (念のため)
	if(nextScene_){
		delete nextScene_;
		nextScene_ = nullptr;
	}
}

// -------------------------------------------------
// メインループ
// -------------------------------------------------

// 更新
void SceneManager::Update(){
	// 次のシーン予約があるなら、切り替えを行う
	if(nextScene_){
		// 1. 旧シーンの終了
		if(scene_){
			scene_->Finalize();
			delete scene_;
		}

		// 2. シーン切り替え
		scene_ = nextScene_;
		nextScene_ = nullptr;

		// 3. 新しいシーンにマネージャー(自分)をセット
		//    (これによりシーン内から ChangeScene が呼べるようになる)
		scene_->SetSceneManager(this);
	}

	// 現在のシーンを実行
	if(scene_){
		scene_->Update();
	}
}

// 描画
void SceneManager::Draw(){
	if(scene_){
		scene_->Draw();
	}
}

// -------------------------------------------------
// シーン制御
// -------------------------------------------------

// 次のシーンを予約する
void SceneManager::ChangeScene(BaseScene* nextScene){
	// 即座に切り替えず、予約変数に入れる
	// (実行中のUpdateが終わった後の、次のフレームの冒頭で切り替わる)
	nextScene_ = nextScene;
}