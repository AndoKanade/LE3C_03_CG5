#include "Framework.h"
#include "Logger.h"

void Framework::Initialize(){
	// ダンプ設定
	SetUnhandledExceptionFilter(Logger::ExportDump);

	// 1. 基盤初期化
	winApi_ = new WinAPI();
	winApi_->Initialize();

	dxCommon_ = new DXCommon();
	dxCommon_->Initialize(winApi_);

	input_ = new Input();
	input_->Initialize(winApi_);

	// 2. マネージャ初期化
	SrvManager::GetInstance()->Initialize(dxCommon_);

#ifdef _DEBUG
	ImGuiManager::GetInstance()->Initialize(winApi_,dxCommon_);
#endif

	SoundManager::GetInstance()->Initialize();
	TextureManager::GetInstance()->Initialize(dxCommon_,SrvManager::GetInstance());
	ModelManager::GetInstance()->Initialize(dxCommon_);
	CameraManager::GetInstance()->Initialize();
	ParticleManager::GetInstance()->Initialize(dxCommon_,SrvManager::GetInstance());

	// 3. 描画共通初期化
	spriteCommon_ = new SpriteCommon();
	object3dCommon_ = new Obj3dCommon();
	object3dCommon_->Initialize(dxCommon_);
}

void Framework::Update(){
	// ウィンドウメッセージ処理
	if(winApi_->ProcessMessage()){
		endRequest_ = true;
	}

	// 入力等の更新
	input_->Update();

#ifdef _DEBUG
	ImGuiManager::GetInstance()->Begin();
#endif

	if(scene_){
		scene_->Update();
	}

	CameraManager::GetInstance()->Update();
}

void Framework::Finalize(){
	// 共通部分の解放
	delete spriteCommon_;
	delete object3dCommon_;
	if(scene_){
		scene_->Finalize(); // シーンの終了処理
		delete scene_;      // メモリ解放
		scene_ = nullptr;
	}


#ifdef _DEBUG
	ImGuiManager::GetInstance()->Finalize();
#endif
	SoundManager::GetInstance()->Finalize();
	ParticleManager::GetInstance()->Finalize();
	ModelManager::GetInstance()->Finalize();
	TextureManager::GetInstance()->Finalize();
	CameraManager::GetInstance()->Finalize();
	SrvManager::GetInstance()->Finalize();

	delete input_;
	delete dxCommon_;
	delete winApi_;
}

// Drawは純粋仮想関数のため、ここには定義不要