#include "Application.h"
#include "SceneManager.h"
#include "SceneFactory.h"

// -------------------------------------------------
// コンストラクタ・デストラクタ
// -------------------------------------------------
Application::Application(){}
Application::~Application(){}

// -------------------------------------------------
// 初期化処理
// -------------------------------------------------
void Application::Initialize(){
	// 1. 基盤(Framework)の初期化
	// (ここでWindow生成、DirectX初期化、Input生成などが行われる)
	Framework::Initialize();

	spriteCommon_ = new SpriteCommon();
	spriteCommon_->Initialize(dxCommon_); // 例


	// 2. シーンマネージャの取得
	sceneManager_ = SceneManager::GetInstance();

	// 3. 共通データの転送 (Dependency Injection)
	// (シーンが生成されたときに渡す Input や Obj3dCommon をマネージャに預ける)
	sceneManager_->SetCommonPtr(object3dCommon_,input_,spriteCommon_);

	// 4. シーン工場の生成とセット
	sceneFactory_ = new SceneFactory();
	sceneManager_->SetFactory(sceneFactory_);

	// 5. 最初のシーンを開始
	sceneManager_->ChangeScene("TITLE");
}

// -------------------------------------------------
// 更新処理
// -------------------------------------------------
void Application::Update(){
	// 1. 基盤更新 (ImGuiManager::Begin など)
	Framework::Update();

	// 2. シーンマネージャの更新
	// (シーンの切り替え処理や、現在のシーンの Update が呼ばれる)
	sceneManager_->Update();

	// 3. ImGui 受付終了
#ifdef _DEBUG
	ImGuiManager::GetInstance()->End();
#endif
}

// -------------------------------------------------
// 描画処理
// -------------------------------------------------
void Application::Draw(){
	// 1. 描画前処理 (画面クリア等)
	dxCommon_->PreDraw();
	SrvManager::GetInstance()->PreDraw();

	// 2. 3D描画共通設定
	object3dCommon_->Draw();

	// 3. 現在のシーンの描画
	sceneManager_->Draw();

	// 4. UI描画 (ImGui)
#ifdef _DEBUG
	ImGuiManager::GetInstance()->Draw();
#endif

	// 5. 描画後処理 (フリップ等)
	dxCommon_->PostDraw();
}

// -------------------------------------------------
// 終了処理
// -------------------------------------------------
void Application::Finalize(){
	// 1. シーン工場の解放
	if(sceneFactory_){
		delete sceneFactory_;
		sceneFactory_ = nullptr;
	}

	if(spriteCommon_) delete spriteCommon_;

	// 2. シーンマネージャの解放
	// (内部で現在のシーンの終了処理なども行われる)
	SceneManager::Destroy();

	// 3. 基盤の終了処理
	Framework::Finalize();
}