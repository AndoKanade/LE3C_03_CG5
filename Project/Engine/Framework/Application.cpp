#include "Application.h"
#include "SceneFactory.h"
#include "SrvManager.h"
#include "TextureManager.h"

// 静的メンバ変数の実体
Application* Application::instance_ = nullptr;
Application::Application(){
	instance_ = this;
}

Application::~Application() = default;

// --- 初期化処理 ---
void Application::Initialize(){
	// 1. 基底クラスの初期化
	Framework::Initialize();
	dxCommon_->InitDepthShaderResourceView();

	// 2. ポストプロセス関連のリソース初期化
	postProcess_ = std::make_unique<PostProcess>();
	postProcess_->Initialize(dxCommon_.get());

	renderTexture_ = std::make_unique<RenderTexture>();

	// RTVハンドルの取得
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dxCommon_->rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += (size_t)dxCommon_->descriptorSizeRTV * 2;

	// SRVハンドルの取得
	uint32_t srvIndex = SrvManager::GetInstance()->Allocate();
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCpu = SrvManager::GetInstance()->GetCPUDescriptorHandle(srvIndex);
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGpu = SrvManager::GetInstance()->GetGPUDescriptorHandle(srvIndex);

	// オフスクリーン用テクスチャの生成
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

	// 3. シーン管理システムの初期化
	sceneFactory_ = std::make_unique<SceneFactory>();
	SceneManager* sceneManager = SceneManager::GetInstance();
	sceneManager->SetFactory(sceneFactory_.get());
	sceneManager->SetCommonPtr(object3dCommon_.get(),input_.get(),spriteCommon_.get());
	sceneManager->ChangeScene("TITLE");

	TextureManager::GetInstance()->LoadTexture("resource/noise0.png");
	TextureManager::GetInstance()->LoadTexture("resource/noise1.png");
	currentMaskPath_ = "resource/noise0.png";
}

void Application::Finalize(){
	Framework::Finalize();
}

void Application::Update(){
	Framework::Update();

	if(isDissolving_){
		// 1フレームの時間を加算
		dissolveTimer_ += 1.0f / 60.0f; // 60FPS固定の場合。実時間を取るならDeltaTimeを使用

		// 0.0 ～ 1.0 に正規化
		float threshold = dissolveTimer_ / kDissolveDuration;

		if(threshold >= 1.0f){
			threshold = 1.0f;
			isDissolving_ = false; // 終了
		}

		// ポストプロセスに値を送る
		postProcess_->SetDissolveThreshold(threshold);
	}

	static float time = 0.0f;
	time += 1.0f / 60.0f;
	postProcess_->SetRandomTime(time);
}

// --- 描画処理 ---
void Application::Draw(){
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

	// 1. RenderTextureへの描画 (パス1：オフスクリーン)
	dxCommon_->PreDraw(renderTexture_.get());
	SrvManager::GetInstance()->PreDraw();

	if(object3dCommon_){
		object3dCommon_->Draw();
	}
	SceneManager::GetInstance()->Draw();

	// 2. Swapchainへの描画 (パス2：ポストプロセス適用)
	dxCommon_->PreDraw(nullptr);

	// リソースバリアの設定：カラーと深度を読み取り状態へ遷移
	D3D12_RESOURCE_BARRIER barriers[2] = {};

	// [0] カラーバッファの遷移
	barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barriers[0].Transition.pResource = renderTexture_->GetResource();
	barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	// [1] 深度バッファの遷移
	barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barriers[1].Transition.pResource = dxCommon_->depthStencilResource.Get();
	barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	commandList->ResourceBarrier(2,barriers);

	// --- マスクハンドルの決定 (Dissolve対応) ---
	// デフォルトは深度テクスチャのハンドルを使用
	D3D12_GPU_DESCRIPTOR_HANDLE secondarySRV = dxCommon_->GetDepthSrvHandleGpu();

	// 現在のポストプロセスがDissolveの場合のみ、マスク用テクスチャ(noise0)に差し替える
	if(currentPPType_ == PostProcess::Type::Dissolve){
		secondarySRV = TextureManager::GetInstance()->GetSrvHandleGPU(currentMaskPath_);
	}

	// ポストプロセスの実行
	// 第3引数(register t1)に、状況に応じたハンドルを渡す
	postProcess_->Draw(commandList,renderTexture_->GetSrvHandleGpu(),secondarySRV,currentPPType_);

	// 次フレームのために状態を元に戻す
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

	// フィルターの切り替え
	const char* typeNames[] = {"Default", "BoxFilter", "Grayscale", "Vignette", "GaussianBlur", "LuminanceOutline", "DepthOutline","RadialBlur","Dissolve","Random"};
	int currentIdx = static_cast<int>(currentPPType_);

	if(ImGui::Combo("Filter Type",&currentIdx,typeNames,IM_ARRAYSIZE(typeNames))){
		currentPPType_ = static_cast<PostProcess::Type>(currentIdx);
	}

	ImGui::Separator();

	// フィルタごとのパラメータ調整
	if(currentPPType_ == PostProcess::Type::Vignette){
		ImGui::Text("Vignette Settings");
		static float intensity = 0.5f;
		if(ImGui::DragFloat("Intensity",&intensity,0.01f,0.0f,1.0f)){
			postProcess_->SetVignetteIntensity(intensity);
		}
		static float scale = 0.8f;
		if(ImGui::DragFloat("Scale",&scale,0.01f,0.0f,2.0f)){
			postProcess_->SetVignetteScale(scale);
		}
	} else if(currentPPType_ == PostProcess::Type::BoxFilter ||
		currentPPType_ == PostProcess::Type::GaussianBlur){
		static int k = 1;
		const char* label = (currentPPType_ == PostProcess::Type::BoxFilter)?"Kernel Size":"Blur Strength";
		if(ImGui::SliderInt(label,&k,0,10)){
			postProcess_->SetKernelSize(k);
		}
	} else if(currentPPType_ == PostProcess::Type::LuminanceOutline ||
		currentPPType_ == PostProcess::Type::DepthOutline){
		ImGui::Text(currentPPType_ == PostProcess::Type::DepthOutline?"Depth Edge Settings":"Luminance Edge Settings");
		ImGui::Text("Edge detection active.");
	} else if(currentPPType_ == PostProcess::Type::RadialBlur){
		ImGui::Text("Radial Blur Settings");

		static float center[2] = {0.5f, 0.5f};
		if(ImGui::DragFloat2("Center",center,0.01f,0.0f,1.0f)){
			postProcess_->SetRadialBlurCenter({center[0], center[1]});
		}

		static float width = 0.01f;
		if(ImGui::DragFloat("Width",&width,0.001f,0.0f,0.1f)){
			postProcess_->SetRadialBlurWidth(width);
		}
	} else if(currentPPType_ == PostProcess::Type::Dissolve){
		ImGui::Text("Dissolve Settings");

		// しきい値 (アニメーション中は外部から更新されるため、ローカルstaticではなくメンバか共有変数にするのが理想)
		// ここではアニメーションの状態を反映させるため、常に最新の値をセットするようにします
		static float threshold = 0.0f;

		// アニメーション中なら強制的に変数を同期（Updateで計算した値をUIにも反映）
		if(isDissolving_){
			threshold = dissolveTimer_ / kDissolveDuration;
		}

		if(ImGui::SliderFloat("Threshold",&threshold,0.0f,1.0f)){
			postProcess_->SetDissolveThreshold(threshold);
		}

		// --- アニメーション制御ボタン ---
		if(ImGui::Button("Start Animation")){
			isDissolving_ = true;
			dissolveTimer_ = 0.0f;
		}
		ImGui::SameLine();
		if(ImGui::Button("Reset")){
			isDissolving_ = false;
			dissolveTimer_ = 0.0f;
			threshold = 0.0f;
			postProcess_->SetDissolveThreshold(0.0f);
		}

		ImGui::Separator();

		// エッジの太さ
		static float edgeWidth = 0.03f;
		if(ImGui::SliderFloat("Edge Width",&edgeWidth,0.0f,0.2f)){
			postProcess_->SetDissolveEdgeWidth(edgeWidth);
		}

		// エッジの色
		static float edgeColor[3] = {1.0f, 0.4f, 0.3f};
		if(ImGui::ColorEdit3("Edge Color",edgeColor)){
			postProcess_->SetDissolveEdgeColor({edgeColor[0], edgeColor[1], edgeColor[2]});
		}

		ImGui::Separator();
		ImGui::Text("Mask Texture");

		const char* masks[] = {"resource/noise0.png", "resource/noise1.png"};
		static int currentMaskIndex = 0;

		if(ImGui::Combo("Select Mask",&currentMaskIndex,masks,IM_ARRAYSIZE(masks))){
			currentMaskPath_ = masks[currentMaskIndex];
		}
	} else if(currentPPType_ == PostProcess::Type::Random){
		ImGui::Text("Random Noise Settings");

		static float intensity = 0.5f;
		if(ImGui::DragFloat("Intensity",&intensity,0.01f,0.0f,1.0f)){
			postProcess_->SetRandomIntensity(intensity);
		}
	}
	ImGui::End();
#endif
}