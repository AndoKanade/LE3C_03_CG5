#include "Application.h"
#include "SceneFactory.h"
#include "SceneManager.h"
#include "ImGuiManager.h" // 必要に応じてinclude

// -------------------------------------------------
// コンストラクタ・デストラクタ
// -------------------------------------------------
Application::Application() = default;
Application::~Application() = default;

// -------------------------------------------------
// 初期化処理
// -------------------------------------------------
void Application::Initialize(){
	// 1. 基底クラス(Framework)の初期化
	// ウィンドウ生成、DirectX初期化、Input、SpriteCommon等の生成が行われます
	Framework::Initialize();

	// 2. シーン工場の生成
	// このアプリケーション専用のシーン生成工場を作成します
	sceneFactory_ = std::make_unique<SceneFactory>();

	// 3. シーンマネージャのセットアップ
	SceneManager* sceneManager = SceneManager::GetInstance();

	// シーン生成工場をセット (AbstractSceneFactoryインターフェースとして渡す)
	sceneManager->SetFactory(sceneFactory_.get());

	// 各シーンで利用する共通システムへのポインタをセット
	// (Frameworkが保持している unique_ptr から生ポインタを取り出して渡す)
	sceneManager->SetCommonPtr(object3dCommon_.get(),input_.get(),spriteCommon_.get());

	// 4. 最初のシーンを開始
	// ここで指定したシーンからゲームが始まります
	sceneManager->ChangeScene("TITLE");
}

// -------------------------------------------------
// 終了処理
// -------------------------------------------------
void Application::Finalize(){
	// 1. 基底クラスの終了処理
	Framework::Finalize();

	// ※ sceneFactory_ 等のメンバ変数は unique_ptr なので、
	// デストラクタで自動的にメモリ解放されます。明示的な delete は不要です。
}

// -------------------------------------------------
// 更新処理
// -------------------------------------------------
void Application::Update(){
	// 1. 基底クラスの更新
	// ウィンドウメッセージ処理、入力更新、シーンマネージャの更新などはここで行われます
	Framework::Update();
}

// -------------------------------------------------
// 描画処理
// -------------------------------------------------
void Application::Draw(){
	// 1. 描画前処理 (DirectXの描画準備)
	dxCommon_->PreDraw();
	SrvManager::GetInstance()->PreDraw();

	// 2. 3Dオブジェクト描画の共通設定
	// ルートシグネチャやパイプラインステートなど、3D描画に必要な設定をコマンドリストに積みます
	if(object3dCommon_){
		object3dCommon_->Draw();
	}

	// 3. 現在のシーンの描画
	// 各シーン内の背景(Sprite)や3Dモデルの描画コマンドを発行します
	SceneManager::GetInstance()->Draw();

	// 4. UI描画 (ImGui)
	// デバッグビルド時のみ有効にするのが一般的です
#ifdef _DEBUG
	ImGuiManager::GetInstance()->Draw();
#endif

	// 5. 描画後処理 (画面のフリップなど)
	dxCommon_->PostDraw();
}