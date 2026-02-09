#include "CameraManager.h"

// ★削除: static変数の初期化は不要
// CameraManager* CameraManager::instance = nullptr;

CameraManager* CameraManager::GetInstance(){
	// ★変更: Meyers Singleton (staticローカル変数)
	// アプリ終了時に自動で破棄されるため、newもdeleteも不要
	static CameraManager instance;
	return &instance;
}

void CameraManager::Finalize(){
	// ★変更: インスタンス自体の delete はしない。
	// 代わりに保持しているカメラをクリアして、参照を切る。
	cameras.clear();
	activeCamera = nullptr;
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
	// (C++20なら contains が使えますが、なければ find != end で代用)
	if(cameras.contains(name)){
		return;
	}

	// 新しいカメラを生成
	// (ここですでに make_unique を使えているのは完璧です！)
	std::unique_ptr<Camera> newCamera = std::make_unique<Camera>();

	// マップに登録 (moveを使って所有権を移動)
	cameras.insert(std::make_pair(name,std::move(newCamera)));

	// もしアクティブカメラがまだ設定されていなければ、これをアクティブにする
	if(activeCamera == nullptr){
		// get() で生ポインタを取得してセット
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