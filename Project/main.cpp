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
#include "Logger.h" // ★追加: Loggerクラスを使用

#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include <iostream>
#include <map>

// ライブラリのリンク設定
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "dinput8.lib")

using namespace DirectX;

// ImGui用
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

#pragma region サウンド関連構造体
struct ChunkHeader{
	char id[4];
	int32_t size;
};

struct RiffHeader{
	ChunkHeader chunk;
	char type[4];
};

struct FormatChunk{
	ChunkHeader chunk;
	WAVEFORMATEX fmt;
};

struct SoundData{
	WAVEFORMATEX wfex;
	BYTE* pBUffer;
	unsigned int bufferSize;
};
#pragma endregion

#pragma region サウンド読み込み関数
SoundData SoundLoadWave(const char* filename){
	std::ifstream file;
	file.open(filename,std::ios_base::binary);
	assert(file.is_open());

	RiffHeader riff;
	file.read((char*)&riff,sizeof(riff));

	if(strncmp(riff.chunk.id,"RIFF",4) != 0){
		assert(0);
	}

	if(strncmp(riff.type,"WAVE",4) != 0){
		assert(0);
	}

	FormatChunk format = {};
	file.read((char*)&format,sizeof(ChunkHeader));

	if(strncmp(format.chunk.id,"fmt ",4) != 0){
		assert(0);
	}

	assert(format.chunk.size <= sizeof(format.fmt));
	file.read((char*)&format.fmt,format.chunk.size);

	ChunkHeader data;
	file.read((char*)&data,sizeof(data));

	// JUNKチャンクがあれば飛ばす
	if(strncmp(data.id,"JUNK",4) == 0){
		file.seekg(data.size,std::ios_base::cur);
		file.read((char*)&data,sizeof(data));
	}

	if(strncmp(data.id,"data",4) != 0){
		assert(0);
	}

	char* pBuffer = new char[data.size];
	file.read(pBuffer,data.size);

	file.close();

	SoundData soundData = {};
	soundData.wfex = format.fmt;
	soundData.pBUffer = reinterpret_cast<BYTE*>(pBuffer);
	soundData.bufferSize = data.size;

	return soundData;
}
#pragma endregion

#pragma region サウンド解放関数
void SoundUnload(SoundData* soundData){
	delete[] soundData->pBUffer;
	soundData->pBUffer = 0;
	soundData->bufferSize = 0;
	soundData->wfex = {};
}
#pragma endregion

#pragma region サウンド再生関数
void SoundPlayWave(IXAudio2* xAudio2,const SoundData& soundData){
	HRESULT result;

	IXAudio2SourceVoice* pSourceVoice = nullptr;
	result = xAudio2->CreateSourceVoice(&pSourceVoice,&soundData.wfex);
	assert(SUCCEEDED(result));

	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.pBUffer;
	buf.AudioBytes = soundData.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;

	result = pSourceVoice->SubmitSourceBuffer(&buf);
	result = pSourceVoice->Start();
}
#pragma endregion

// ★ Log関数は Logger.h にあるので削除しました

#pragma region ダンプ出力
static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception){
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = {0};
	CreateDirectory(L"./Dumps",nullptr);
	StringCchPrintfW(filePath,MAX_PATH,L"./Dumps/%04d-%02d%02d-%02d%02d.dmp",
		time.wYear,time.wMonth,time.wDay,time.wHour,
		time.wMinute);
	HANDLE dumpFileHandle =
		CreateFile(filePath,GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_WRITE | FILE_SHARE_READ,0,CREATE_ALWAYS,0,0);

	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();

	MINIDUMP_EXCEPTION_INFORMATION minidumpinformation{0};
	minidumpinformation.ThreadId = threadId;
	minidumpinformation.ExceptionPointers = exception;
	minidumpinformation.ClientPointers = TRUE;

	MiniDumpWriteDump(GetCurrentProcess(),processId,dumpFileHandle,
		MiniDumpNormal,&minidumpinformation,nullptr,nullptr);

	return EXCEPTION_EXECUTE_HANDLER;
}
#pragma endregion


// ===================================================================================
// メイン関数
// ===================================================================================
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE,LPSTR,int){
	D3DResourceLeakChecker leakCheck;

	// 例外発生時のダンプ出力設定
	SetUnhandledExceptionFilter(ExportDump);

	// 1. ウィンドウ初期化
	WinAPI* winApi = new WinAPI();
	winApi->Initialize();

	// 2. DirectX初期化
	DXCommon* dxCommon = new DXCommon();
	dxCommon->Initialize(winApi);

	// 3. 入力初期化
	Input* input = new Input();
	input->Initialize(winApi);

	// 4. オーディオ初期化 (XAudio2)
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
	IXAudio2MasteringVoice* masterVoice = nullptr;
	HRESULT result = XAudio2Create(&xAudio2,0,XAUDIO2_DEFAULT_PROCESSOR);
	result = xAudio2->CreateMasteringVoice(&masterVoice);
	assert(SUCCEEDED(result));

	// 音声データの読み込み
	SoundData soundData = SoundLoadWave("resource/You_and_Me.wav");


	// 5. テクスチャマネージャ初期化
	TextureManager::GetInstance()->Initialize(dxCommon);
	// 必要なテクスチャをロード
	TextureManager::GetInstance()->LoadTexture("resource/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("resource/monsterball.png");


	// 6. スプライト共通部 初期化
	SpriteCommon* spriteCommon = new SpriteCommon();
	spriteCommon->Initialize(dxCommon);

	// スプライト生成
	Sprite* sprite = new Sprite();
	sprite->Initialize(spriteCommon,"resource/uvChecker.png");

	Sprite* spriteBall = new Sprite();
	spriteBall->Initialize(spriteCommon,"resource/monsterball.png");
	spriteBall->SetPosition({200.0f, 0.0f});


	// ==============================================================================
	// 7. 3Dモデル関連の初期化
	// ==============================================================================

	// (A) 3Dオブジェクト共通部
	Obj3dCommon* object3dCommon = new Obj3dCommon();
	object3dCommon->Initialize(dxCommon);

	// (B) モデルマネージャ初期化
	ModelManager::GetInstance()->Initialize(dxCommon);

	// (C) モデルデータの読み込み
	ModelManager::GetInstance()->LoadModel("plane.obj");
	ModelManager::GetInstance()->LoadModel("fence.obj");


	// ==============================================================================
	// 8. 3Dオブジェクト(インスタンス)の生成
	// ==============================================================================

	// 1つ目のオブジェクト (plane)
	Obj3D* object3d = new Obj3D();
	object3d->Initialize(object3dCommon);
	object3d->SetModel("plane.obj");
	object3d->SetTranslate({0.0f, 0.0f, 0.0f});
	object3d->SetScale({1.0f, 1.0f, 1.0f});

	// 2つ目のオブジェクト (axis)
	Obj3D* object3d_2 = new Obj3D();
	object3d_2->Initialize(object3dCommon);
	// ★ axis.obj をセット！
	object3d_2->SetModel("fence.obj");
	object3d_2->SetTranslate({2.0f, 0.0f, 0.0f});
	object3d_2->SetScale({1.0f, 1.0f, 1.0f});


	// ==============================================================================
	// ゲームループ
	// ==============================================================================
	while(true){
		// メッセージ処理 (×ボタンで終了)
		if(winApi->ProcessMessage()){
			break;
		} else{
			// --- 更新処理 ---
			input->Update();

			// 3Dオブジェクト更新
			object3d->Update();
			object3d_2->Update();

			// スプライト更新
			sprite->Update();
			spriteBall->Update();

			// スペースキーでログ出力テスト
			if(input->TriggerKey(DIK_SPACE)){
				Logger::Log("Trigger space\n"); // ★ Logger::Logを使用
				// SoundPlayWave(xAudio2.Get(), soundData);
			}


			// --- 描画処理 ---
			dxCommon->PreDraw(); // 描画開始

			// 1. 3D描画
			object3dCommon->Draw(); // 共通設定
			object3d->Draw();       // plane描画
			object3d_2->Draw();     // axis描画

			// 2. スプライト描画
			spriteCommon->Draw();   // 共通設定
			sprite->Draw();
			// spriteBall->Draw();

			dxCommon->PostDraw(); // 描画終了
		}
	}

	Logger::Log("Game Loop Finished.\n");


	// ==============================================================================
	// 終了処理 (解放)
	// ==============================================================================

	// テクスチャマネージャ解放
	TextureManager::GetInstance()->Finalize();

	// モデルマネージャ解放
	ModelManager::GetInstance()->Finalize();

	// 3Dオブジェクト解放
	delete object3d;
	delete object3d_2;
	delete object3dCommon;

	// スプライト解放
	delete spriteBall;
	delete sprite;
	delete spriteCommon;

	// システム解放
	delete dxCommon;
	delete input;

	// オーディオ解放
	xAudio2.Reset();
	SoundUnload(&soundData);

	// ウィンドウ解放
	winApi->Finalize();
	delete winApi;

	return 0;
}