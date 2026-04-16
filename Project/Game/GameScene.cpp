#include "GameScene.h"

// --- エンジン/システム関連 ---
#include "CameraManager.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "ParticleManager.h"
#include "SoundManager.h"
#include "TextureManager.h"

// --- ゲームオブジェクト関連 ---
// Obj3Dなどの実体を使うため、必ずインクルードする
#include "Input.h"
#include "Obj3D.h"
#include "Obj3dCommon.h"
#include "ParticleEmitter.h"
#include "SpriteCommon.h"

// --- 定数定義 ---
namespace{
	// ファイルパス
	const std::string kTextureChecker = "resource/uvChecker.png";
	const std::string kTextureBall = "resource/monsterball.png";
	const std::string kTextureCircle = "resource/Circle.png";

	const std::string kModelPlane = "plane.obj";
	const std::string kModelFence = "fence.obj";
	const std::string kModelSphere = "sphere.obj";

	const std::string kParticleName = "Circle";
}

// コンストラクタ
GameScene::GameScene() = default;

// デストラクタ
GameScene::~GameScene() = default;

// 終了処理
void GameScene::Finalize(){
	// unique_ptr が自動的にメモリを解放するため、明示的な処理は不要です
}

// 初期化処理
void GameScene::Initialize(Obj3dCommon* object3dCommon,Input* input,SpriteCommon* spriteCommon){
	// 1. システムポインタの保持
	object3dCommon_ = object3dCommon;
	input_ = input;
	spriteCommon_ = spriteCommon;

	// 2. リソースのロード
	// テクスチャ
	TextureManager::GetInstance()->LoadTexture(kTextureChecker);
	TextureManager::GetInstance()->LoadTexture(kTextureBall);
	TextureManager::GetInstance()->LoadTexture(kTextureCircle);

	// モデル
	ModelManager::GetInstance()->LoadModel(kModelPlane);
	ModelManager::GetInstance()->LoadModel(kModelFence);
	ModelManager::GetInstance()->LoadModel(kModelSphere);

	// サウンド
	SoundManager::GetInstance()->SoundLoadFile(kBgmPath_);

	// パーティクル設定
	ParticleManager::GetInstance()->CreateParticleGroup(kParticleName,kTextureCircle);

	// 3. オブジェクトの生成と初期化

	// --- 親オブジェクト: 地面 (Plane) ---
	planeObj_ = std::make_unique<Obj3D>();
	planeObj_->Initialize(object3dCommon_);
	planeObj_->SetModel(kModelPlane);

	// --- 子オブジェクト: 柵 (Fence) ---
	fenceObj_ = std::make_unique<Obj3D>();
	fenceObj_->Initialize(object3dCommon_);
	fenceObj_->SetModel(kModelFence);

	sphereObj_ = std::make_unique<Obj3D>();
	sphereObj_->Initialize(object3dCommon_);
	sphereObj_->SetModel(kModelSphere);

	// 親子付け設定
	// unique_ptrから生ポインタ(.get())を取り出して親としてセットします
	fenceObj_->SetParent(planeObj_);

	// 座標設定 (親である Plane からの相対座標)
	fenceObj_->SetTranslate({2.0f, 0.0f, 0.0f});

	// --- パーティクルエミッタ ---
	Transform emitterConfig;
	emitterConfig.scale = {1.0f, 1.0f, 1.0f};
	// 名前、Transform、数、寿命を指定して生成
	particleEmitter_ = std::make_unique<ParticleEmitter>(kParticleName,emitterConfig,10,0.2f);

	// 4. カメラの設定
	CameraManager::GetInstance()->CreateCamera("default");
	auto* defaultCamera = CameraManager::GetInstance()->GetCamera("default");

	if(defaultCamera){
		defaultCamera->SetTranslate({0.0f, 0.0f, -10.0f});
		CameraManager::GetInstance()->SetActiveCamera("default");
	}

	// 3Dオブジェクト共通設定にアクティブカメラを登録
	object3dCommon_->SetDefaultCamera(CameraManager::GetInstance()->GetActiveCamera());
}

// 更新処理
void GameScene::Update(){

	// 1. オブジェクトの更新
	if(sphereObj_){
		sphereObj_->Update();
	}

	// 2. パーティクルの更新
	if(particleEmitter_){ particleEmitter_->Update(); }

	// 3. サウンド処理 (スペースキーで再生)
	if(input_->TriggerKey(DIK_SPACE)){
		if(!SoundManager::GetInstance()->IsPlaying(kBgmPath_)){
			SoundManager::GetInstance()->PlayAudio(kBgmPath_,0.5f,true);
		}
	}

	// 4. ImGui (デバッグ用)
#ifdef USE_IMGUI
	Camera* activeCamera = CameraManager::GetInstance()->GetActiveCamera();
	if(activeCamera){
		ImGui::Begin("GameScene Debug");

		// カメラ位置調整
		Vector3 camPos = activeCamera->GetTranslate();
		ImGui::DragFloat3("Camera Pos",&camPos.x,0.1f);
		activeCamera->SetTranslate(camPos);

		// 親オブジェクト(Plane)の操作
		if(planeObj_){
			Vector3 pPos = planeObj_->GetTranslate();
			ImGui::DragFloat3("Parent(Plane) Pos",&pPos.x,0.1f);
			planeObj_->SetTranslate(pPos);
		}
		if(sphereObj_){
			ImGui::Separator();
			ImGui::Text("Sphere Object");

			// 回転 (Rotation)
			Vector3 sRot = sphereObj_->GetRotate();
			// 回転は見やすいように少し感度を変えることもあります (0.1f -> 1.0fなど)
			if(ImGui::DragFloat3("Sphere Rotate",&sRot.x,0.1f)){
				sphereObj_->SetRotate(sRot);
			}
		}

		ImGui::End();
	}
#endif
}

// 描画処理
void GameScene::Draw(){
	// オブジェクト描画
	if(sphereObj_){ sphereObj_->Draw(); }
}