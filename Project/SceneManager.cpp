#include "SceneManager.h"
#include <cassert>

// -------------------------------------------------
// 静的メンバ変数の実体定義
// -------------------------------------------------
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
	// ※ sceneFactory_ は Framework 側で管理・削除するため、ここでは delete しない
}

// -------------------------------------------------
// メインループ
// -------------------------------------------------

// 更新処理
void SceneManager::Update(){
	// 次のシーン予約があるなら (予約名が空でなければ) 切り替え実行
	if(nextSceneName_ != ""){

		// 1. 旧シーンの終了
		if(scene_){
			scene_->Finalize();
			delete scene_;
			scene_ = nullptr;
		}

		// 2. ファクトリーを使って新しいシーンを生成
		if(sceneFactory_){
			scene_ = sceneFactory_->CreateScene(nextSceneName_);
		}

		// 3. 新しいシーンの初期化と依存性の注入
		if(scene_){
			// マネージャをセット (シーン内から ChangeScene を呼べるようにする)
			scene_->SetSceneManager(this);

			// 共通データ (Obj3dCommon, Input) を渡して初期化
			// これを行わないとシーン内でメンバ変数が nullptr になりクラッシュする
			if(object3dCommon_ && input_){
				scene_->Initialize(object3dCommon_,input_);
			}
		}

		// 予約をクリア
		nextSceneName_ = "";
	}

	// 現在のシーンを実行
	if(scene_){
		scene_->Update();
	}
}

// 描画処理
void SceneManager::Draw(){
	if(scene_){
		scene_->Draw();
	}
}

// -------------------------------------------------
// シーン制御
// -------------------------------------------------

// シーン切り替え予約
void SceneManager::ChangeScene(const std::string& sceneName){
	// ファクトリーがセットされていなければエラー
	assert(sceneFactory_);

	// 次のシーン名を予約 (実際の生成・切り替えは Update の冒頭で行う)
	nextSceneName_ = sceneName;
}