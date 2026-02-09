#include "Application.h"
#include "SceneManager.h"
#include "SceneFactory.h" // ★追加: make_unique<SceneFactory>のため

// -------------------------------------------------
// コンストラクタ・デストラクタ
// -------------------------------------------------
Application::Application() = default;
Application::~Application() = default;

// -------------------------------------------------
// 初期化処理
// -------------------------------------------------
void Application::Initialize(){
	// 1. 基盤(Framework)の初期化
	// ★これで WinAPI, DXCommon, Input, SpriteCommon などが
	//   すべて make_unique され、準備完了になります。
	Framework::Initialize();

	// ★削除: spriteCommon_ = 
	// SpriteCommon(); 
	// (Framework::Initialize 内ですでに生成されているため、ここで new するとメモリリークします！)


	// 2. シーン工場の生成とセット
	// Framework が持っている unique_ptr に、具体的な工場(SceneFactory)を入れます
	sceneFactory_ = std::make_unique<SceneFactory>();


	// 3. シーンマネージャのセットアップ
	SceneManager* sceneManager = SceneManager::GetInstance();

	// 工場を渡す (.get() で unique_ptr から生ポインタを取り出す)
	sceneManager->SetFactory(sceneFactory_.get());

	// 共通データを渡す (Framework が持っている unique_ptr から .get() する)
	sceneManager->SetCommonPtr(object3dCommon_.get(),input_.get(),spriteCommon_.get());


	// 4. 最初のシーンを開始
	sceneManager->ChangeScene("TITLE");
}

// -------------------------------------------------
// 更新処理
// -------------------------------------------------
void Application::Update(){
	// 1. 基盤更新
	// (この中で Windowメッセージ処理、Input更新、SceneManager::Update が行われます)
	Framework::Update();

	// ★削除: sceneManager_->Update();
	// (Framework::Update 内で呼ばれるようになったので、ここで呼ぶと2回更新されてしまいます)

	// ImGuiの終了処理などは Framework や Draw で完結させるのが一般的です
}

// -------------------------------------------------
// 描画処理
// -------------------------------------------------
void Application::Draw(){
	// 1. 描画前処理
	dxCommon_->PreDraw();
	SrvManager::GetInstance()->PreDraw();

	// ★★★ ここが抜けています！ ★★★
	// 3Dオブジェクトを描画する前に、共通の「ルートシグネチャ」と「パイプライン」をセットする必要があります。
	// Obj3dCommon にそのような関数があるはずです。（名前は実装によりますが、恐らく Draw か CommonDraw です）
	object3dCommon_->Draw();  // ← これを追加！(関数名は Obj3dCommon.h を確認してください)

	// 2. シーンの描画
	SceneManager::GetInstance()->Draw();

	// 3. UI描画 (ImGui)
#ifdef _DEBUG
	ImGuiManager::GetInstance()->Draw();
#endif

	// 4. 描画後処理
	dxCommon_->PostDraw();
}

// -------------------------------------------------
// 終了処理
// -------------------------------------------------
void Application::Finalize(){
	// ★変更: delete はすべて削除！
	// sceneFactory_, spriteCommon_ などは Framework のデストラクタで
	// unique_ptr が自動的にメモリ解放してくれます。

	// 1. 基盤の終了処理
	Framework::Finalize();

	// (SceneManager::Destroy も Framework::Finalize 内で呼ぶように修正済なら削除OKですが、
	//  念のためここで呼んでも安全です)
}