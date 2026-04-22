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
	Framework::Initialize();

	// --- RenderTextureの生成と初期化 ---
	renderTexture_ = std::make_unique<RenderTexture>();

	// RTVハンドルの取得 (バックバッファ2つの次、インデックス2を使用)
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dxCommon_->rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += (size_t)dxCommon_->descriptorSizeRTV * 2;

	// SRVハンドルの取得 (SrvManagerから空きを確保)
	uint32_t srvIndex = SrvManager::GetInstance()->Allocate();
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCpu = SrvManager::GetInstance()->GetCPUDescriptorHandle(srvIndex);
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGpu = SrvManager::GetInstance()->GetGPUDescriptorHandle(srvIndex);

	// クリアカラー（資料通りの青っぽい色か、一旦わかりやすく赤系など）
	Vector4 clearColor = {0.1f, 0.25f, 0.5f, 1.0f};

	// 生成実行
	renderTexture_->Create(
		dxCommon_->GetDevice(),
		WinAPI::kClientWidth,
		WinAPI::kCliantHeight,
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		clearColor,
		rtvHandle,
		srvHandleCpu,
		srvHandleGpu
	);

	// 2. シーン工場の生成
	sceneFactory_ = std::make_unique<SceneFactory>();

	// 3. シーンマネージャのセットアップ
	SceneManager* sceneManager = SceneManager::GetInstance();
	sceneManager->SetFactory(sceneFactory_.get());
	sceneManager->SetCommonPtr(object3dCommon_.get(),input_.get(),spriteCommon_.get());

	// 4. 最初のシーンを開始
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
	// --- パス1：RenderTexture への描画 (画面には出ない) ---
	// 引数に renderTexture を渡して、描き込み先を切り替える
	dxCommon_->PreDraw(renderTexture_.get());
	SrvManager::GetInstance()->PreDraw();

	if(object3dCommon_){
		object3dCommon_->Draw();
	}

	// モデルや背景の描画（RenderTexture に書き込まれる）
	SceneManager::GetInstance()->Draw();

	// --- パス2：Swapchain への描画 (最終的な画面出力) ---
	// 引数なし(nullptr)で呼び出し、描き込み先をバックバッファに戻す
	dxCommon_->PreDraw();

	// ここで資料の通り、ImGui だけを描画する
#ifdef _DEBUG
	ImGuiManager::GetInstance()->Draw();
#endif

	// 画面フリップ
	dxCommon_->PostDraw();
}