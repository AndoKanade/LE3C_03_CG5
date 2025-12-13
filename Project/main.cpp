#define _USE_MATH_DEFINES
#define PI 3.14159265f
#include "DXCommon.h"
#include "Input.h"
#include "StringUtility.h"
#include "WinAPI.h"
#include <chrono>
#include <cmath>
#include <dbghelp.h>
#include <dxcapi.h>
#include <dxgidebug.h>
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>
#include <string.h>
#include <strsafe.h>
#include <vector>
#include <xaudio2.h>

// エンジン内ヘッダー
#include "SpriteCommon.h"
#include "Sprite.h"
#include "Math.h"
#include "Obj3DCommon.h"
#include "Obj3D.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "Logger.h"
#include "Camera.h"
#include "CameraManager.h"

#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include <iostream>
#include <map>
#include "SrvManager.h"

// ライブラリリンク
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "dinput8.lib")

using namespace DirectX;

// ImGui用プロシージャハンドラ
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lPalam);

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam){
	if(ImGui_ImplWin32_WndProcHandler(hwnd,msg,wparam,lparam)){
		return true;
	}
	switch(msg){
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(hwnd,msg,wparam,lparam);
}

// リソースリークチェッカー
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

// ==========================================================================
// サウンド関連 (構造体・関数)
// ==========================================================================
#pragma region サウンド処理
struct ChunkHeader{ char id[4]; int32_t size; };
struct RiffHeader{ ChunkHeader chunk; char type[4]; };
struct FormatChunk{ ChunkHeader chunk; WAVEFORMATEX fmt; };
struct SoundData{ WAVEFORMATEX wfex; BYTE* pBUffer; unsigned int bufferSize; };

SoundData SoundLoadWave(const char* filename){
	std::ifstream file;
	file.open(filename,std::ios_base::binary);
	assert(file.is_open());

	RiffHeader riff;
	file.read((char*)&riff,sizeof(riff));
	if(strncmp(riff.chunk.id,"RIFF",4) != 0) assert(0);
	if(strncmp(riff.type,"WAVE",4) != 0) assert(0);

	FormatChunk format = {};
	file.read((char*)&format,sizeof(ChunkHeader));
	if(strncmp(format.chunk.id,"fmt ",4) != 0) assert(0);
	assert(format.chunk.size <= sizeof(format.fmt));
	file.read((char*)&format.fmt,format.chunk.size);

	ChunkHeader data;
	file.read((char*)&data,sizeof(data));
	if(strncmp(data.id,"JUNK",4) == 0){
		file.seekg(data.size,std::ios_base::cur);
		file.read((char*)&data,sizeof(data));
	}
	if(strncmp(data.id,"data",4) != 0) assert(0);

	char* pBuffer = new char[data.size];
	file.read(pBuffer,data.size);
	file.close();

	SoundData soundData = {};
	soundData.wfex = format.fmt;
	soundData.pBUffer = reinterpret_cast<BYTE*>(pBuffer);
	soundData.bufferSize = data.size;
	return soundData;
}

void SoundUnload(SoundData* soundData){
	delete[] soundData->pBUffer;
	soundData->pBUffer = 0;
	soundData->bufferSize = 0;
	soundData->wfex = {};
}

void SoundPlayWave(IXAudio2* xAudio2,const SoundData& soundData){
	HRESULT result;
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	result = xAudio2->CreateSourceVoice(&pSourceVoice,&soundData.wfex);
	assert(SUCCEEDED(result));

	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.pBUffer;
	buf.AudioBytes = soundData.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;
	pSourceVoice->SubmitSourceBuffer(&buf);
	pSourceVoice->Start();
}
#pragma endregion

#pragma region ダンプ出力
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
#pragma endregion


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

	SrvManager* srvManager = SrvManager::GetInstance();
	srvManager->Initialize(dxCommon);

	// オーディオ (XAudio2)
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
	IXAudio2MasteringVoice* masterVoice = nullptr;
	HRESULT result = XAudio2Create(&xAudio2,0,XAUDIO2_DEFAULT_PROCESSOR);
	result = xAudio2->CreateMasteringVoice(&masterVoice);
	assert(SUCCEEDED(result));

	// 音声データのロード
	SoundData soundData = SoundLoadWave("resource/You_and_Me.wav");

	// ---------------------------------------------------------------
	// 2. リソースマネージャ・共通部分の初期化
	// ---------------------------------------------------------------

	// テクスチャ
	TextureManager::GetInstance()->Initialize(dxCommon,srvManager);
	TextureManager::GetInstance()->LoadTexture("resource/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("resource/monsterball.png");

	// スプライト共通
	SpriteCommon* spriteCommon = new SpriteCommon();
	spriteCommon->Initialize(dxCommon);

	// 3Dオブジェクト共通
	Obj3dCommon* object3dCommon = new Obj3dCommon();
	object3dCommon->Initialize(dxCommon);

	// モデルマネージャ
	ModelManager::GetInstance()->Initialize(dxCommon);
	ModelManager::GetInstance()->LoadModel("plane.obj"); // plane読み込み
	ModelManager::GetInstance()->LoadModel("fence.obj"); // fence読み込み


	// ---------------------------------------------------------------
	// 3. ゲームオブジェクトの生成
	// ---------------------------------------------------------------

	// スプライト
	Sprite* sprite = new Sprite();
	sprite->Initialize(spriteCommon,"resource/uvChecker.png");

	Sprite* spriteBall = new Sprite();
	spriteBall->Initialize(spriteCommon,"resource/monsterball.png");
	spriteBall->SetPosition({200.0f, 0.0f});

	// 3Dオブジェクト 1 (plane)
	Obj3D* object3d = new Obj3D();
	object3d->Initialize(object3dCommon);
	object3d->SetModel("plane.obj");
	object3d->SetTranslate({0.0f, 0.0f, 0.0f});
	object3d->SetScale({1.0f, 1.0f, 1.0f});

	// 3Dオブジェクト 2 (fence)
	Obj3D* object3d_2 = new Obj3D();
	object3d_2->Initialize(object3dCommon);
	object3d_2->SetModel("fence.obj");
	object3d_2->SetTranslate({2.0f, 0.0f, 0.0f});
	object3d_2->SetScale({1.0f, 1.0f, 1.0f});


	// ---------------------------------------------------------------
	// 4. カメラの初期化
	// ---------------------------------------------------------------
	CameraManager::GetInstance()->Initialize();

	// デフォルトカメラ作成
	CameraManager::GetInstance()->CreateCamera("default");
	auto* defaultCamera = CameraManager::GetInstance()->GetCamera("default");
	defaultCamera->SetRotate({0.0f, 0.0f, 0.0f});
	defaultCamera->SetTranslate({0.0f, 0.0f, -10.0f});

	// アクティブ設定
	CameraManager::GetInstance()->SetActiveCamera("default");

	// 共通部にセット
	object3dCommon->SetDefaultCamera(CameraManager::GetInstance()->GetActiveCamera());


	// ==============================================================================
	// ゲームループ
	// ==============================================================================
	while(true){
		if(winApi->ProcessMessage()){
			break;
		} else{
			// -----------------------------------------
			// 更新処理
			// -----------------------------------------
			input->Update();

			// --- カメラ移動処理 (WASD) ---
			// マネージャから現在のアクティブカメラを取得して操作する
			Camera* activeCamera = CameraManager::GetInstance()->GetActiveCamera();
			if(activeCamera){
				const float kCameraSpeed = 0.1f;
				Vector3 translate = activeCamera->GetTranslate();

				if(input->PushKey(DIK_W)){ translate.z += kCameraSpeed; } // 奥へ
				if(input->PushKey(DIK_S)){ translate.z -= kCameraSpeed; } // 手前へ
				if(input->PushKey(DIK_A)){ translate.x -= kCameraSpeed; } // 左へ
				if(input->PushKey(DIK_D)){ translate.x += kCameraSpeed; } // 右へ

				activeCamera->SetTranslate(translate);
			}

			// マネージャ更新 (カメラ行列の計算など)
			CameraManager::GetInstance()->Update();

			// 念のため毎フレームセット (カメラ切替に対応するため)
			object3dCommon->SetDefaultCamera(CameraManager::GetInstance()->GetActiveCamera());


			// オブジェクト更新
			object3d->Update();
			object3d_2->Update();
			sprite->Update();
			spriteBall->Update();

			// テスト機能
			if(input->TriggerKey(DIK_SPACE)){
				Logger::Log("Trigger space\n");
				// SoundPlayWave(xAudio2.Get(), soundData);
			}


			// -----------------------------------------
			// 描画処理
			// -----------------------------------------
			dxCommon->PreDraw();
			srvManager->PreDraw();

			// 3D描画
			object3dCommon->Draw(); // 共通設定
			object3d->Draw();       // plane
			object3d_2->Draw();     // fence

			// 2D描画
			spriteCommon->Draw();   // 共通設定
			sprite->Draw();
			// spriteBall->Draw();

			dxCommon->PostDraw();
		}
	}

	Logger::Log("Game Loop Finished.\n");


	// ==============================================================================
	// 終了処理 (解放)
	// ==============================================================================

	// マネージャ解放
	SrvManager::GetInstance()->Finalize();
	TextureManager::GetInstance()->Finalize();
	ModelManager::GetInstance()->Finalize();
	CameraManager::GetInstance()->Finalize();


	// オブジェクト解放
	delete object3d;
	delete object3d_2;
	delete object3dCommon;

	delete spriteBall;
	delete sprite;
	delete spriteCommon;

	// 基盤解放
	delete dxCommon;
	delete input;

	// サウンド解放
	xAudio2.Reset();
	SoundUnload(&soundData);

	// ウィンドウ解放
	winApi->Finalize();
	delete winApi;

	return 0;
}