#include "GameScene.h"
#include "CameraManager.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "ParticleManager.h"
#include "SoundManager.h"
#include "TextureManager.h"
#include "Skybox.h"
#include "SkyboxCommon.h"
#include "Input.h"
#include "Obj3D.h"
#include "Obj3dCommon.h"
#include "ParticleEmitter.h"
#include "SpriteCommon.h"
#include "Application.h"

namespace{
	const std::string kTextureChecker = "resource/uvChecker.png";
	const std::string kTextureBall = "resource/monsterball.png";
	const std::string kTextureCircle = "resource/Circle.png";
	const std::string kSkyboxTexture = "resource/Skybox/rostock_laage_airport_4k.dds";

	const std::string kModelPlane = "plane.obj";
	const std::string kModelFence = "fence.obj";
	const std::string kModelSphere = "sphere.obj";

	const std::string kParticleName = "Circle";
}

GameScene::GameScene() = default;
GameScene::~GameScene() = default;

void GameScene::Initialize(Obj3dCommon* object3dCommon,Input* input,SpriteCommon* spriteCommon){
	object3dCommon_ = object3dCommon;
	input_ = input;
	spriteCommon_ = spriteCommon;

	// リソースロード
	TextureManager::GetInstance()->LoadTexture(kTextureChecker);
	TextureManager::GetInstance()->LoadTexture(kTextureBall);
	TextureManager::GetInstance()->LoadTexture(kTextureCircle);
	TextureManager::GetInstance()->LoadTexture(kSkyboxTexture);

	ModelManager::GetInstance()->LoadModel(kModelPlane);
	ModelManager::GetInstance()->LoadModel(kModelFence);
	ModelManager::GetInstance()->LoadModel(kModelSphere);

	SoundManager::GetInstance()->SoundLoadFile(kBgmPath_);

	// オブジェクト生成
	planeObj_ = std::make_unique<Obj3D>();
	planeObj_->Initialize(object3dCommon_);
	planeObj_->SetModel(kModelPlane);

	fenceObj_ = std::make_unique<Obj3D>();
	fenceObj_->Initialize(object3dCommon_);
	fenceObj_->SetModel(kModelFence);
	fenceObj_->SetParent(planeObj_);
	fenceObj_->SetTranslate({2.0f, 0.0f, 0.0f});

	sphereObj_ = std::make_unique<Obj3D>();
	sphereObj_->Initialize(object3dCommon_);
	sphereObj_->SetModel(kModelSphere);

	skyboxCommon_ = std::make_unique<SkyboxCommon>();
	skyboxCommon_->Initialize(object3dCommon_->GetDxCommon());
	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize(skyboxCommon_.get(),kSkyboxTexture);

	// パーティクル設定
	ParticleManager::GetInstance()->CreateParticleGroup(kParticleName,kTextureCircle);
	Transform emitterConfig;
	emitterConfig.scale = {1.0f, 1.0f, 1.0f};
	particleEmitter_ = std::make_unique<ParticleEmitter>(kParticleName,emitterConfig,10,0.2f);

	// カメラ設定
	CameraManager::GetInstance()->CreateCamera("default");
	auto* defaultCamera = CameraManager::GetInstance()->GetCamera("default");
	if(defaultCamera){
		defaultCamera->SetTranslate({0.0f, 0.0f, -10.0f});
		CameraManager::GetInstance()->SetActiveCamera("default");
	}
	object3dCommon_->SetDefaultCamera(CameraManager::GetInstance()->GetActiveCamera());
}

void GameScene::Finalize(){}

void GameScene::Update(){
	if(sphereObj_){
		sphereObj_->Update();
	}

	if(skybox_){
		skybox_->Update(*CameraManager::GetInstance()->GetActiveCamera());
	}

	if(particleEmitter_){
		particleEmitter_->Update();
	}

	if(input_->TriggerKey(DIK_SPACE)){
		if(!SoundManager::GetInstance()->IsPlaying(kBgmPath_)){
			SoundManager::GetInstance()->PlayAudio(kBgmPath_,0.5f,true);
		}
	}

#ifdef USE_IMGUI
	// --- シーン独自のデバッグUI ---
	Camera* activeCamera = CameraManager::GetInstance()->GetActiveCamera();
	if(activeCamera){
		ImGui::Begin("GameScene Debug");

		Vector3 camPos = activeCamera->GetTranslate();
		if(ImGui::DragFloat3("Camera Pos",&camPos.x,0.1f)){
			activeCamera->SetTranslate(camPos);
		}

		Vector3 camRot = activeCamera->GetRotate();
		if(ImGui::DragFloat3("Camera Rotate",&camRot.x,0.01f)){
			activeCamera->SetRotate(camRot);
		}

		if(planeObj_){
			Vector3 pPos = planeObj_->GetTranslate();
			ImGui::DragFloat3("Parent(Plane) Pos",&pPos.x,0.1f);
			planeObj_->SetTranslate(pPos);
		}

		if(sphereObj_){
			ImGui::Separator();
			ImGui::Text("Sphere Object");
			Vector3 sRot = sphereObj_->GetRotate();
			if(ImGui::DragFloat3("Sphere Rotate",&sRot.x,0.1f)){
				sphereObj_->SetRotate(sRot);
			}
		}

		ImGui::End();
	}

	// --- アプリケーション共通のポストプロセスUI ---
	Application::GetInstance()->ShowPostProcessUI();
#endif
}

void GameScene::Draw(){
	object3dCommon_->Draw();

	if(sphereObj_){
		sphereObj_->Draw();
	}

	// if(skybox_){
	//	skybox_->Draw();
	// }
}