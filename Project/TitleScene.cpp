#include "TitleScene.h"
#include "SceneManager.h" 
#include "ModelManager.h"
#include "Input.h"

// デストラクタ
TitleScene::~TitleScene(){
	Finalize();
}

// 終了処理
void TitleScene::Finalize(){
	// メモリ解放 (delete後は nullptr で安全策)
	if(titleObject_){
		delete titleObject_;
		titleObject_ = nullptr;
	}
}

// 初期化
void TitleScene::Initialize(Obj3dCommon* object3dCommon,Input* input){
	object3dCommon_ = object3dCommon;
	input_ = input;

	// --- リソースのロード ---
	ModelManager::GetInstance()->LoadModel("fence.obj");

	// --- オブジェクト生成 ---
	titleObject_ = new Obj3D();
	titleObject_->Initialize(object3dCommon_);
	titleObject_->SetModel("fence.obj");

	// 座標とサイズの設定
	titleObject_->SetTranslate({0.0f, 0.0f, 0.0f});
	titleObject_->SetScale({0.5f, 0.5f, 0.5f});
}

// 更新
void TitleScene::Update(){
	// オブジェクトの更新
	if(titleObject_){
		titleObject_->Update();
	}

	// シーン遷移処理 (スペースキー)
	if(input_->TriggerKey(DIK_SPACE)){
	
		// 3. マネージャーに切り替え依頼
		SceneManager::GetInstance()->ChangeScene("GAME");
	}
}

// 描画
void TitleScene::Draw(){
	if(titleObject_){
		titleObject_->Draw();
	}
}