#define _USE_MATH_DEFINES
#define PI 3.14159265f

// --- エンジン・ライブラリ関連ヘッダー ---
#include "DXCommon.h"
#include "Input.h"
#include "StringUtility.h"
#include "WinAPI.h"
#include "Math.h"
#include "Logger.h"

// --- マネージャクラス ---
#include "SrvManager.h"
#include "ImGuiManager.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "SpriteCommon.h"
#include "Obj3DCommon.h"
#include "CameraManager.h"
#include "ParticleManager.h"
#include "SoundManager.h" // 追加

// --- ゲームオブジェクト ---
#include "Sprite.h"
#include "Obj3D.h"
#include "Camera.h"
#include "ParticleEmitter.h"

// --- その他 ---
#include <chrono>
#include <cmath>
#include <dbghelp.h>
#include <dxcapi.h>
#include <dxgidebug.h>
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <strsafe.h>

// --- ライブラリリンク ---
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "dinput8.lib")

using namespace DirectX;

// ==========================================================================
// ImGui プロシージャハンドラ
// ==========================================================================
#ifdef _DEBUG
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lPalam);
#endif

// ==========================================================================
// ウィンドウプロシージャ
// ==========================================================================
LRESULT CALLBACK WindowProc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam){
#ifdef _DEBUG
	if(ImGui_ImplWin32_WndProcHandler(hwnd,msg,wparam,lparam)){
		return true;
	}
#endif
	switch(msg){
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(hwnd,msg,wparam,lparam);
}

// ==========================================================================
// デバッグ・エラーハンドリング
// ==========================================================================
struct D3DResourceLeakChecker{
	~D3DResourceLeakChecker(){
		Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
		if(SUCCEEDED(DXGIGetDebugInterface1(0,IID_PPV_ARGS(&debug)))){
			debug->ReportLiveObjects(DXGI_DEBUG_ALL,DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP,DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12,DXGI_DEBUG_RLO_ALL);
		}
	}
};

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception){
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = {0};
	CreateDirectory(L"./Dumps",nullptr);
	StringCchPrintfW(filePath,MAX_PATH,L"./Dumps/%04d-%02d%02d-%02d%02d.dmp",
		time.wYear,time.wMonth,time.wDay,time.wHour,time.wMinute);
	HANDLE dumpFileHandle = CreateFile(filePath,GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_WRITE | FILE_SHARE_READ,0,CREATE_ALWAYS,0,0);

	MINIDUMP_EXCEPTION_INFORMATION minidumpinformation{0};
	minidumpinformation.ThreadId = GetCurrentThreadId();
	minidumpinformation.ExceptionPointers = exception;
	minidumpinformation.ClientPointers = TRUE;

	MiniDumpWriteDump(GetCurrentProcess(),GetCurrentProcessId(),dumpFileHandle,
		MiniDumpNormal,&minidumpinformation,nullptr,nullptr);
	return EXCEPTION_EXECUTE_HANDLER;
}

// ==========================================================================
// メイン関数
// ==========================================================================
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE,LPSTR,int){
	D3DResourceLeakChecker leakCheck;
	SetUnhandledExceptionFilter(ExportDump);

	// ---------------------------------------------------------------
	// 1. システム・基盤部分の初期化
	// ---------------------------------------------------------------
	WinAPI* winApi = new WinAPI();
	winApi->Initialize();

	DXCommon* dxCommon = new DXCommon();
	dxCommon->Initialize(winApi);

	Input* input = new Input();
	input->Initialize(winApi);

	// --- マネージャ群の初期化 ---
	// ※ 依存関係に注意 (SrvManagerは早めに)
	SrvManager* srvManager = SrvManager::GetInstance();
	srvManager->Initialize(dxCommon);

	// Debugビルド時のみImGuiを初期化
#ifdef _DEBUG
	ImGuiManager::GetInstance()->Initialize(winApi,dxCommon);
#endif

	// サウンドマネージャ
	SoundManager::GetInstance()->Initialize();

	// リソース系マネージャ
	TextureManager::GetInstance()->Initialize(dxCommon,srvManager);
	ModelManager::GetInstance()->Initialize(dxCommon);
	CameraManager::GetInstance()->Initialize();
	ParticleManager::GetInstance()->Initialize(dxCommon,srvManager);

	// 描画共通クラス
	SpriteCommon* spriteCommon = new SpriteCommon();
	// spriteCommon->Initialize(dxCommon); // 必要ならコメントイン

	Obj3dCommon* object3dCommon = new Obj3dCommon();
	object3dCommon->Initialize(dxCommon);

	// ---------------------------------------------------------------
	// 2. リソースのロード
	// ---------------------------------------------------------------
	// テクスチャ
	TextureManager::GetInstance()->LoadTexture("resource/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("resource/monsterball.png");

	// モデル
	ModelManager::GetInstance()->LoadModel("plane.obj");
	ModelManager::GetInstance()->LoadModel("fence.obj");
	ModelManager::GetInstance()->LoadModel("sphere.obj");

	// パーティクルリソース
	ParticleManager::GetInstance()->CreateParticleGroup("Circle","resource/Circle.png");

	// サウンド
	const std::string kBgmPath = "resource/You_and_Me.wav"; // 定数化
	SoundManager::GetInstance()->LoadWave(kBgmPath);


	// ---------------------------------------------------------------
	// 3. ゲームオブジェクトの生成・初期設定
	// ---------------------------------------------------------------

	// 3Dオブジェクト
	Obj3D* planeObj = new Obj3D();
	planeObj->Initialize(object3dCommon);
	planeObj->SetModel("plane.obj");
	planeObj->SetScale({1.0f, 1.0f, 1.0f});

	Obj3D* fenceObj = new Obj3D();
	fenceObj->Initialize(object3dCommon);
	fenceObj->SetModel("fence.obj");
	fenceObj->SetTranslate({2.0f, 0.0f, 0.0f});

	// パーティクルエミッター
	Transform emitterConfig;
	emitterConfig.scale = {1.0f, 1.0f, 1.0f};
	emitterConfig.translate = {0.0f, 0.0f, 0.0f};
	ParticleEmitter* particleEmitter = new ParticleEmitter("Circle",emitterConfig,10,0.2f);

	// カメラ設定
	CameraManager::GetInstance()->CreateCamera("default");
	auto* defaultCamera = CameraManager::GetInstance()->GetCamera("default");
	defaultCamera->SetTranslate({0.0f, 0.0f, -10.0f});
	CameraManager::GetInstance()->SetActiveCamera("default");
	object3dCommon->SetDefaultCamera(CameraManager::GetInstance()->GetActiveCamera());

	// その他変数
	float lightDir[3] = {0.0f, -1.0f, 0.0f};
	bool isPaused = false;

	// ==============================================================================
	// ゲームループ
	// ==============================================================================
	while(true){
		// ウィンドウメッセージ処理
		if(winApi->ProcessMessage()){
			break;
		}

		// -----------------------------------------
		// 1. 更新処理 (Update)
		// -----------------------------------------
		input->Update();

		// --- ImGui 開始 ---
#ifdef USE_IMGUI
		ImGuiManager::GetInstance()->Begin();
#endif

		// --- カメラ制御 ---
		Camera* activeCamera = CameraManager::GetInstance()->GetActiveCamera();
		if(activeCamera){
			Vector3 translate = activeCamera->GetTranslate();

#ifdef USE_IMGUI
			ImGui::Begin("Camera Control");
			ImGui::DragFloat3("Position",&translate.x,0.1f);
			ImGui::End();
#endif
			activeCamera->SetTranslate(translate);
		}

		// マネージャ更新
		CameraManager::GetInstance()->Update();
		object3dCommon->SetDefaultCamera(CameraManager::GetInstance()->GetActiveCamera());

		// オブジェクト更新
		planeObj->SetLightDirection({lightDir[0], lightDir[1], lightDir[2]});
		planeObj->Update();
		// fenceObj->Update(); // 必要なら追加

		// --- サウンド入力処理 ---

		// スペースキー: 再生
		if(input->TriggerKey(DIK_SPACE)){
			Logger::Log("Play Sound\n");
			// 再生中でなければ再生する
			if(!SoundManager::GetInstance()->IsPlaying(kBgmPath)){
				SoundManager::GetInstance()->PlayWave(kBgmPath,0.5f,true);
			}
		}

		// エンターキー: 停止
		if(input->TriggerKey(DIK_RETURN)){
			Logger::Log("Stop Sound\n");
			SoundManager::GetInstance()->StopWave(kBgmPath);
		}

		// Pキー: ポーズ切り替え
		if(input->TriggerKey(DIK_P)){
			if(isPaused){
				SoundManager::GetInstance()->ResumeWave(kBgmPath);
				isPaused = false;
				Logger::Log("Resume Sound\n");
			} else{
				SoundManager::GetInstance()->PauseWave(kBgmPath);
				isPaused = true;
				Logger::Log("Pause Sound\n");
			}
		}

		// --- ImGui 終了 ---
#ifdef USE_IMGUI
		ImGuiManager::GetInstance()->End();
#endif

		// -----------------------------------------
		// 2. 描画処理 (Draw)
		// -----------------------------------------
		dxCommon->PreDraw();
		srvManager->PreDraw();

		// 3D描画
		object3dCommon->Draw();
		planeObj->Draw();
		// fenceObj->Draw(); // 必要なら描画

		// ImGui描画
#ifdef USE_IMGUI
		ImGuiManager::GetInstance()->Draw();
#endif

		dxCommon->PostDraw();
	}

	Logger::Log("Game Loop Finished.\n");

	// ==============================================================================
	// 終了処理 (解放)
	// ==============================================================================
	// 生成順の逆で解放するのが安全です

	// マネージャの終了処理
#ifdef USE_IMGUI
	ImGuiManager::GetInstance()->Finalize();
#endif
	SoundManager::GetInstance()->Finalize();
	ParticleManager::GetInstance()->Finalize();
	ModelManager::GetInstance()->Finalize();
	TextureManager::GetInstance()->Finalize();
	CameraManager::GetInstance()->Finalize();
	SrvManager::GetInstance()->Finalize();

	// ゲームオブジェクト解放
	delete particleEmitter;
	delete planeObj;
	delete fenceObj;
	delete object3dCommon;
	delete spriteCommon;

	// 基盤解放
	delete input;
	delete dxCommon;
	delete winApi;

	return 0;
}