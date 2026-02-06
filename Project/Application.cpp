#include "Application.h"
#include "TitleScene.h"

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
	Framework::Initialize();

	// 2. シーンマネージャの取得
	sceneManager_ = SceneManager::GetInstance();

	// 3. 最初のシーン(TitleScene)を生成
	BaseScene* scene = new TitleScene();

	// ★重要：シーンの初期化 (データを渡す)
	scene->Initialize(object3dCommon_,input_);

	// 4. マネージャーにシーンをセット
	sceneManager_->ChangeScene(scene);
}

// -------------------------------------------------
// 更新処理
// -------------------------------------------------
void Application::Update(){
	// 1. 基盤更新 (ImGuiManager::Begin などが含まれる)
	Framework::Update();

	// 2. シーンマネージャの更新 (シーン切り替えや現在のシーンのUpdate)
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
	// 1. 描画前処理
	dxCommon_->PreDraw();
	SrvManager::GetInstance()->PreDraw();

	// 2. 3D描画共通設定 (ルートシグネチャの設定など)
	object3dCommon_->Draw();

	// 3. 現在のシーンの描画
	sceneManager_->Draw();

	// 4. UI描画 (ImGui)
#ifdef _DEBUG
	ImGuiManager::GetInstance()->Draw();
#endif

	// 5. 描画後処理
	dxCommon_->PostDraw();
}

// -------------------------------------------------
// 終了処理
// -------------------------------------------------
void Application::Finalize(){
	// 1. シーンマネージャの解放
	// (Frameworkより先に消さないと、リソース解放順序でエラーになる場合があるため)
	SceneManager::Destroy();

	// 2. 基盤の終了処理
	Framework::Finalize();
}