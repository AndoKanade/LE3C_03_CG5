#include "GameScene.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "SoundManager.h"
#include "ParticleManager.h"
#include "CameraManager.h"
#include "ImGuiManager.h"

// ★追加: コンストラクタ (LNK2001エラー対策)
GameScene::GameScene() = default;

// ★変更: デストラクタ (C2027エラー対策)
// ここで unique_ptr の中身 (Obj3DやParticleEmitter) が自動削除されます。
GameScene::~GameScene(){}

// 終了処理
void GameScene::Finalize(){
	// ★変更: delete を全て削除！
	// unique_ptr が勝手にメモリ解放してくれるので、何も書かなくてOKです。
}

// 初期化
void GameScene::Initialize(Obj3dCommon* object3dCommon,Input* input,SpriteCommon* spriteCommon){
	// メンバ変数に保存
	object3dCommon_ = object3dCommon;
	input_ = input;
	spriteCommon_ = spriteCommon;

	/// -------------------------------------------
	/// 1. リソースのロード (テクスチャ・モデル・音)
	/// -------------------------------------------
	TextureManager::GetInstance()->LoadTexture("resource/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("resource/monsterball.png");
	TextureManager::GetInstance()->LoadTexture("resource/Circle.png");

	ModelManager::GetInstance()->LoadModel("plane.obj");
	ModelManager::GetInstance()->LoadModel("fence.obj");
	ModelManager::GetInstance()->LoadModel("sphere.obj");

	SoundManager::GetInstance()->SoundLoadFile(kBgmPath_);

	// パーティクルグループの作成
	ParticleManager::GetInstance()->CreateParticleGroup("Circle","resource/Circle.png");

	/// -------------------------------------------
	/// 2. オブジェクトの生成と初期化
	/// -------------------------------------------

	// 地面 (Plane)
	// ★変更: make_unique に置き換え
	planeObj_ = std::make_unique<Obj3D>();
	planeObj_->Initialize(object3dCommon_);
	planeObj_->SetModel("plane.obj");

	// 柵 (Fence)
	// ★変更: make_unique に置き換え
	fenceObj_ = std::make_unique<Obj3D>();
	fenceObj_->Initialize(object3dCommon_);
	fenceObj_->SetModel("fence.obj");
	fenceObj_->SetTranslate({2.0f, 0.0f, 0.0f});

	// パーティクルエミッタ
	Transform emitterConfig;
	emitterConfig.scale = {1.0f, 1.0f, 1.0f};

	// ★変更: make_unique に置き換え (引数はそのまま渡せます)
	particleEmitter_ = std::make_unique<ParticleEmitter>("Circle",emitterConfig,10,0.2f);

	/// -------------------------------------------
	/// 3. カメラの設定
	/// -------------------------------------------
	CameraManager::GetInstance()->CreateCamera("default");
	auto* defaultCamera = CameraManager::GetInstance()->GetCamera("default");
	defaultCamera->SetTranslate({0.0f, 0.0f, -10.0f});
	CameraManager::GetInstance()->SetActiveCamera("default");

	// 3Dオブジェクト共通設定にカメラをセット
	object3dCommon_->SetDefaultCamera(CameraManager::GetInstance()->GetActiveCamera());
}

// 更新
void GameScene::Update(){
	// オブジェクトの更新
	if(planeObj_){ planeObj_->Update(); }
	if(fenceObj_){ fenceObj_->Update(); }

	// パーティクルなど他の更新があればここに記述
	if(particleEmitter_){ particleEmitter_->Update(); }

	// BGM再生 (スペースキー)
	if(input_->TriggerKey(DIK_SPACE)){
		if(!SoundManager::GetInstance()->IsPlaying(kBgmPath_)){
			SoundManager::GetInstance()->PlayAudio(kBgmPath_,0.5f,true);
		}
	}

	// ImGui (デバッグ用)
#ifdef USE_IMGUI
	Camera* activeCamera = CameraManager::GetInstance()->GetActiveCamera();
	if(activeCamera){
		ImGui::Begin("GameScene Debug");

		// カメラ位置調整
		Vector3 translate = activeCamera->GetTranslate();
		ImGui::DragFloat3("Camera Pos",&translate.x,0.1f);
		activeCamera->SetTranslate(translate);

		ImGui::End();
	}
#endif
}

// 描画
void GameScene::Draw(){
	// オブジェクトの描画
	if(planeObj_){ planeObj_->Draw(); }
	if(fenceObj_){ fenceObj_->Draw(); }

	// パーティクルの描画が必要ならここに追加
}