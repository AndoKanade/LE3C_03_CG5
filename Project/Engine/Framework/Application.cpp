#include "Application.h"
#include "SceneFactory.h"
#include "SrvManager.h"

// 静的メンバ変数の実体定義
Application* Application::instance_ = nullptr;

Application::Application(){
	instance_ = this; // シングルトンインスタンスの登録
}

Application::~Application() = default;

// --- 初期化処理 ---
void Application::Initialize(){
	// 1. 基底クラスの初期化
	Framework::Initialize();
	dxCommon_->InitDepthShaderResourceView(); 

	// 2. ポストプロセス用リソースの初期化
	postProcess_ = std::make_unique<PostProcess>();
	postProcess_->Initialize(dxCommon_.get());

	renderTexture_ = std::make_unique<RenderTexture>();

	// RTVハンドルの取得 (バックバッファ2つの次、インデックス2を使用)
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dxCommon_->rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += (size_t)dxCommon_->descriptorSizeRTV * 2;

	// SRVハンドルの取得 (SrvManagerから空きを確保)
	uint32_t srvIndex = SrvManager::GetInstance()->Allocate();
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCpu = SrvManager::GetInstance()->GetCPUDescriptorHandle(srvIndex);
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGpu = SrvManager::GetInstance()->GetGPUDescriptorHandle(srvIndex);

	// レンダーテクスチャ生成
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

	// --- パス1：RenderTextureへの描画 (オフスクリーン) ---
	dxCommon_->PreDraw(renderTexture_.get());
	SrvManager::GetInstance()->PreDraw();

	if(object3dCommon_){
		object3dCommon_->Draw();
	}
	SceneManager::GetInstance()->Draw();

	// --- パス2：Swapchainへの描画 (ポストプロセス適用) ---
	dxCommon_->PreDraw(nullptr); // 描画先をバックバッファに切り替え

	// 1. カラーと深度の状態を「テクスチャ用(SRV)」へ一括遷移
	D3D12_RESOURCE_BARRIER barriers[2] = {};

	// [0] RenderTexture (カラーバッファ)
	barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barriers[0].Transition.pResource = renderTexture_->GetResource();
	barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	// [1] DepthBuffer (深度バッファ) ★追加
	barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barriers[1].Transition.pResource = dxCommon_->depthStencilResource.Get(); // 深度リソース
	barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	commandList->ResourceBarrier(2,barriers);

	// 2. ポストプロセス実行
	postProcess_->Draw(commandList,renderTexture_->GetSrvHandleGpu(),dxCommon_->GetDepthSrvHandleGpu(),currentPPType_);

	// 3. 次のフレームのために状態を元に戻す
	barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	commandList->ResourceBarrier(2,barriers);

#ifdef _DEBUG
	ImGuiManager::GetInstance()->Draw();
#endif

	dxCommon_->PostDraw();
}

// --- デバッグUI表示 ---
void Application::ShowPostProcessUI(){
#ifdef _DEBUG
	ImGui::Begin("PostProcess Settings");

	// フィルター切り替え
	const char* typeNames[] = {"Default(PostProcess)", "BoxFilter", "Grayscale", "Vignette", "GaussianBlur", "Outline"};
	int currentIdx = static_cast<int>(currentPPType_);

	if(ImGui::Combo("Filter Type",&currentIdx,typeNames,IM_ARRAYSIZE(typeNames))){
		currentPPType_ = static_cast<PostProcess::Type>(currentIdx);
	}

	ImGui::Separator();

	// パラメータ調整
	if(currentPPType_ == PostProcess::Type::Vignette){
		ImGui::Text("Vignette Settings");

		static float intensity = 0.5f;
		if(ImGui::DragFloat("Intensity",&intensity,0.01f,0.0f,1.0f)){
			postProcess_->SetVignetteIntensity(intensity);
		}

		static float scale = 0.8f;
		// 修正：SetKernelSize になっていたのを SetVignetteScale に変更
		if(ImGui::DragFloat("Scale",&scale,0.01f,0.0f,2.0f)){
			postProcess_->SetVignetteScale(scale);
		}
	} else if(currentPPType_ == PostProcess::Type::BoxFilter){
		static int k = 1;
		if(ImGui::SliderInt("Kernel Size (k)##PostProcess",&k,0,5)){
			postProcess_->SetKernelSize(k);
		}
	} else if(currentPPType_ == PostProcess::Type::GaussianBlur){
		static int k = 2;
		if(ImGui::SliderInt("Blur Strength##Gaussian",&k,0,10)){
			postProcess_->SetKernelSize(k);
		}
	}else if(currentPPType_ == PostProcess::Type::Outline){
		// アウトラインフィルターのパラメータ調整
	}

	ImGui::End();
#endif
}