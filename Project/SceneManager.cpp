#include "SceneManager.h"
#include <cassert>

// -------------------------------------------------
// 静的メンバ変数の定義 (削除)
// -------------------------------------------------
// ★削除: Meyers Singletonにするため、ポインタ変数の定義は不要です
// SceneManager* SceneManager::instance_ = nullptr;

// -------------------------------------------------
// シングルトン管理
// -------------------------------------------------

// インスタンス取得
SceneManager* SceneManager::GetInstance(){
	// ★変更: staticローカル変数 (自動生成・自動破棄)
	static SceneManager instance;
	return &instance;
}

// 終了処理 (Application::Finalize で呼ばれる)
void SceneManager::Finalize(){
	// 最後のシーンを終了させる
	if(scene_){
		scene_->Finalize();
	}
	// unique_ptr をリセット（破棄）
	scene_.reset();
}

// -------------------------------------------------
// コンストラクタ・デストラクタ
// -------------------------------------------------
// ★デストラクタはヘッダーで default にしているので、ここには書きません
// (unique_ptr が勝手にメモリを解放してくれるため、明示的な delete scene_ は不要です)


// -------------------------------------------------
// メインループ
// -------------------------------------------------

// 更新処理
void SceneManager::Update(){
	// 次のシーン予約があるなら切り替え実行
	if(nextSceneName_ != ""){

		// 1. 旧シーンの終了
		if(scene_){
			scene_->Finalize();
		}

		// 2. ファクトリーを使って新しいシーンを生成
		if(sceneFactory_){
			// ★変更: rawポインタを unique_ptr に所有させるために .reset() を使います
			// CreateScene が生ポインタ(BaseScene*)を返すと仮定しています
			scene_ = sceneFactory_->CreateScene(nextSceneName_);
		}

		// 3. 新しいシーンの初期化と依存性の注入
		if(scene_){
			// マネージャをセット
			scene_->SetSceneManager(this);

			// 共通データを渡して初期化
			// (object3dCommon_ などがセットされているか確認)
			if(object3dCommon_ && input_ && spriteCommon_){
				// BaseScene::Initialize の引数に合わせて調整してください
				// (SetCommonPtrで渡されたものをそのまま渡す)
				scene_->Initialize(object3dCommon_,input_,spriteCommon_);
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
	assert(sceneFactory_);
	nextSceneName_ = sceneName;
}