#include "Framework.h"
#include "Logger.h"
#include "SceneManager.h" // シーンの更新・描画をここに任せるため

// unique_ptr用
#include <memory>

void Framework::Initialize(){
	// ダンプ設定
	SetUnhandledExceptionFilter(Logger::ExportDump);

	// 1. 基盤初期化
	// make_unique で生成
	winApi_ = std::make_unique<WinAPI>();
	winApi_->Initialize();

	dxCommon_ = std::make_unique<DXCommon>();
	// 他のクラスが生ポインタを欲しがる場合は .get() で渡す
	dxCommon_->Initialize(winApi_.get());

	input_ = std::make_unique<Input>();
	input_->Initialize(winApi_.get());

	// 2. マネージャ初期化
	SrvManager::GetInstance()->Initialize(dxCommon_.get());

#ifdef _DEBUG
	ImGuiManager::GetInstance()->Initialize(winApi_.get(),dxCommon_.get());
#endif

	SoundManager::GetInstance()->Initialize();

	// .get() を使って渡す
	TextureManager::GetInstance()->Initialize(dxCommon_.get(),SrvManager::GetInstance());
	ModelManager::GetInstance()->Initialize(dxCommon_.get());
	CameraManager::GetInstance()->Initialize();
	ParticleManager::GetInstance()->Initialize(dxCommon_.get(),SrvManager::GetInstance());

	// 3. 描画共通初期化
	// SpriteCommonの初期化を忘れずに！
	spriteCommon_ = std::make_unique<SpriteCommon>();
	spriteCommon_->Initialize(dxCommon_.get());

	object3dCommon_ = std::make_unique<Obj3dCommon>();
	object3dCommon_->Initialize(dxCommon_.get());
}

void Framework::Update(){
	// ウィンドウメッセージ処理
	if(winApi_->ProcessMessage()){
		endRequest_ = true;
	}

	// 入力等の更新
	input_->Update();

	// ★順番変更: ImGuiのフレーム開始処理を先に実行！
	// これを先にやらないと、シーンの中で ImGui::Begin() を呼んだ時に落ちます
#ifdef _DEBUG
	ImGuiManager::GetInstance()->Begin();
#endif

	// ★ここに移動: ImGuiの準備ができてからシーンを更新
	SceneManager::GetInstance()->Update();
#ifdef _DEBUG
	ImGuiManager::GetInstance()->End();
#endif

	// カメラの更新
	CameraManager::GetInstance()->Update();
}

void Framework::Finalize(){
	// unique_ptr が自動で解放してくれるので delete は不要

	// シーンマネージャの終了処理
	SceneManager::GetInstance()->Finalize();
#ifdef _DEBUG
	ImGuiManager::GetInstance()->Finalize();
#endif
	SoundManager::GetInstance()->Finalize();
	ParticleManager::GetInstance()->Finalize();
	ModelManager::GetInstance()->Finalize();
	TextureManager::GetInstance()->Finalize();
	CameraManager::GetInstance()->Finalize();
	SrvManager::GetInstance()->Finalize();

	// input_, dxCommon_, winApi_ も自動で消えるので delete 不要
}