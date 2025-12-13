#include "CameraManager.h"

// staticメンバ変数の初期化
CameraManager* CameraManager::instance = nullptr;

CameraManager* CameraManager::GetInstance(){
	if(instance == nullptr){
		instance = new CameraManager();
	}
	return instance;
}

void CameraManager::Finalize(){
	if(instance != nullptr){
		delete instance;
		instance = nullptr;
	}
}

void CameraManager::Initialize(){
	// 念のためコンテナをクリア
	cameras.clear();
	activeCamera = nullptr;
}

void CameraManager::Update(){
	// アクティブなカメラがあれば更新する
	if(activeCamera){
		activeCamera->Update();
	}
}

void CameraManager::CreateCamera(const std::string& name){
	// すでに同じ名前があれば何もしない
	if(cameras.contains(name)){
		return;
	}

	// 新しいカメラを生成
	std::unique_ptr<Camera> newCamera = std::make_unique<Camera>();

	// マップに登録
	cameras.insert(std::make_pair(name,std::move(newCamera)));

	// もしアクティブカメラがまだ設定されていなければ、これをアクティブにする
	if(activeCamera == nullptr){
		activeCamera = cameras[name].get();
	}
}

void CameraManager::SetActiveCamera(const std::string& name){
	// 指定された名前のカメラが存在するか確認
	if(cameras.contains(name)){
		activeCamera = cameras[name].get();
	}
}

Camera* CameraManager::GetActiveCamera() const{
	return activeCamera;
}

Camera* CameraManager::GetCamera(const std::string& name){
	if(cameras.contains(name)){
		return cameras.at(name).get();
	}
	return nullptr;
}