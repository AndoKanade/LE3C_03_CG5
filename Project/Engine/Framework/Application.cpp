#include "Application.h"
#include "SceneFactory.h"
#include "SrvManager.h"

// 静的メンバ変数の実体定義
Application* Application::instance_ = nullptr;

Application::Application(){
	// インスタンスを登録 (GetInstance用)
	instance_ = this;
}

Application::~Application() = default;

void Application::Initialize(){
	// 1. 基底クラスの初期化
	Framework::Initialize();

	// 2. RenderTextureの生成と初期化
	renderTexture_ = std::make_unique<RenderTexture>();

	// RTVハンドルの取得 (バックバッファ2つの次、インデックス2を使用)
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dxCommon_->rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += (size_t)dxCommon_->descriptorSizeRTV * 2;

	// ポストプロセス初期化
	postProcess_ = std::make_unique<PostProcess>();
	postProcess_->Initialize(dxCommon_.get());

	// SRVハンドルの取得 (SrvManagerから空きを確保)
	uint32_t srvIndex = SrvManager::GetInstance()->Allocate();
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCpu = SrvManager::GetInstance()->GetCPUDescriptorHandle(srvIndex);
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGpu = SrvManager::GetInstance()->GetGPUDescriptorHandle(srvIndex);

	renderTexture_->Create(
		dxCommon_->GetDevice(),
		WinAPI::kClientWidth,
		WinAPI::kCliantHeight,
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		{0.1f, 0.25f, 0.5f, 1.0f},
		rtvHandle,
		srvHandleCpu,
		srvHandleGpu
	);

	// 3. シーン系の初期化
	sceneFactory_ = std::make_unique<SceneFactory>();
	SceneManager* sceneManager = SceneManager::GetInstance();
	sceneManager->SetFactory(sceneFactory_.get());
	sceneManager->SetCommonPtr(object3dCommon_.get(),input_.get(),spriteCommon_.get());
	sceneManager->ChangeScene("TITLE");
}

void Application::Finalize(){
	Framework::Finalize();
}

void Application::Update(){
	Framework::Update();
}

void Application::Draw(){
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

	// --- パス1：RenderTextureへの描画 ---
	dxCommon_->PreDraw(renderTexture_.get());
	SrvManager::GetInstance()->PreDraw();

	if(object3dCommon_){
		object3dCommon_->Draw();
	}
	SceneManager::GetInstance()->Draw();

	// --- パス2：Swapchainへの描画 ---
	// 描画先をバックバッファに切り替え
	dxCommon_->PreDraw(nullptr);

	// RenderTextureをテクスチャとして使うためのバリア (RT -> SRV)
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = renderTexture_->GetResource();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	commandList->ResourceBarrier(1,&barrier);

	// ポストプロセス描画
	postProcess_->Draw(commandList,renderTexture_->GetSrvHandleGpu());

	// 次のフレームのためにバリアを戻す (SRV -> RT)
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	commandList->ResourceBarrier(1,&barrier);

#ifdef _DEBUG
	ImGuiManager::GetInstance()->Draw();
#endif

	dxCommon_->PostDraw();
}

void Application::ShowPostProcessUI(){
#ifdef _DEBUG
	ImGui::Begin("PostProcess Settings");

	static int k = 1;
	// IDの衝突を避けるために ##PostProcess を付与
	if(ImGui::SliderInt("Blur Strength##PostProcess",&k,0,5)){
		if(postProcess_){
			postProcess_->SetKernelSize(k);
		}
	}

	ImGui::End();
#endif
}