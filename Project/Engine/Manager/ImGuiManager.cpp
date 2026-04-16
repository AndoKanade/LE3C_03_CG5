#include "ImGuiManager.h"

// プロジェクト内ヘッダー
#include "WinAPI.h"
#include "DXCommon.h"
#include "SrvManager.h"

// ImGuiライブラリ
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

// 安全確認用
#include <cassert>

// ==========================================================================
// シングルトンインスタンスの取得
// ==========================================================================
ImGuiManager* ImGuiManager::GetInstance(){
	static ImGuiManager instance;
	return &instance;
}

// ==========================================================================
// 初期化処理
// ==========================================================================
void ImGuiManager::Initialize([[maybe_unused]] WinAPI* winApi,[[maybe_unused]] DXCommon* dxCommon){
#ifdef USE_IMGUI

	// メンバ変数の保存
	dxCommon_ = dxCommon;

	// --- 依存オブジェクトのNullチェック (安全対策) ---
	assert(winApi && "WinAPI instance is null.");
	assert(dxCommon && "DXCommon instance is null.");
	assert(dxCommon->GetCommandQueue() && "CommandQueue is null. Check DXCommon initialization.");

	// 1. ImGuiコンテキストの作成
	ImGui::CreateContext();

	// 2. スタイルの設定
	ImGui::StyleColorsDark();

	// 3. Win32初期化
	ImGui_ImplWin32_Init(winApi->GetHwnd());

	// 4. SRVヒープ領域の確保 (フォントテクスチャ用)
	SrvManager* srvManager = SrvManager::GetInstance();
	assert(srvManager && "SrvManager instance is null.");

	srvIndex_ = srvManager->Allocate();
	srvHeap_ = srvManager->GetDescriptorHeap();

	// 5. DirectX12初期化設定
	ImGui_ImplDX12_InitInfo initInfo = {};

	initInfo.Device = dxCommon_->GetDevice();
	initInfo.CommandQueue = dxCommon_->GetCommandQueue();
	initInfo.NumFramesInFlight = static_cast<int>(dxCommon_->GetSwapChainResourcesNum());

	// エラー回避のためのフォーマット設定 (SRGB)
	initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN; // 深度バッファは使用しない

	// ヒープ設定 (SrvManagerのものを使用)
	initInfo.SrvDescriptorHeap = srvHeap_;
	initInfo.LegacySingleSrvCpuDescriptor = srvManager->GetCPUDescriptorHandle(srvIndex_);
	initInfo.LegacySingleSrvGpuDescriptor = srvManager->GetGPUDescriptorHandle(srvIndex_);

	// 初期化実行
	ImGui_ImplDX12_Init(&initInfo);

#endif
}

// ==========================================================================
// フレーム開始処理
// ==========================================================================
void ImGuiManager::Begin(){
#ifdef USE_IMGUI
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
#endif
}

// ==========================================================================
// フレーム終了処理 (内部コマンド生成)
// ==========================================================================
void ImGuiManager::End(){
#ifdef USE_IMGUI
	ImGui::Render();
#endif
}

// ==========================================================================
// 描画処理 (GPUコマンド発行)
// ==========================================================================
void ImGuiManager::Draw(){
#ifdef USE_IMGUI

	// コマンドリスト取得
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

	// ディスクリプタヒープのセット
	// ※SrvManager管理の共通ヒープを使用する (ゲーム内テクスチャと共存させるため)
	ID3D12DescriptorHeap* ppHeaps[] = {SrvManager::GetInstance()->GetDescriptorHeap()};
	commandList->SetDescriptorHeaps(_countof(ppHeaps),ppHeaps);

	// 描画コマンド発行
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(),commandList);

#endif
}

// ==========================================================================
// 終了処理
// ==========================================================================
void ImGuiManager::Finalize(){
#ifdef USE_IMGUI
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif
}