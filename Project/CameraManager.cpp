#include "CameraManager.h"

/// <summary>
/// シングルトンインスタンスの取得
/// Meyers Singleton パターンを採用。
/// 初回呼び出し時に static 変数が初期化され、プログラム終了時に自動でデストラクタが呼ばれます。
/// </summary>
CameraManager* CameraManager::GetInstance(){
	static CameraManager instance;
	return &instance;
}

/// <summary>
/// 初期化
/// </summary>
void CameraManager::Initialize(){
	cameras_.clear();
	activeCamera_ = nullptr;
}

/// <summary>
/// 終了処理
/// </summary>
void CameraManager::Finalize(){
	// コンテナをクリアすることで、unique_ptr が管理する Camera インスタンスも破棄される
	cameras_.clear();
	activeCamera_ = nullptr;
}

/// <summary>
/// 更新処理
/// </summary>
void CameraManager::Update(){
	// アクティブなカメラがあれば更新処理を回す
	if(activeCamera_){
		activeCamera_->Update();
	}
}

/// <summary>
/// カメラ作成
/// </summary>
void CameraManager::CreateCamera(const std::string& name){
	// すでに同じ名前のカメラが存在する場合は何もしない
	// (C++20 以降なら contains が使用可能。C++17以前なら cameras_.find(name) != cameras_.end() を使用)
	if(cameras_.contains(name)){
		return;
	}

	// 新しいカメラを生成 (make_unique推奨)
	std::unique_ptr<Camera> newCamera = std::make_unique<Camera>();

	// マップに登録 (moveを使って所有権をコンテナへ移動)
	cameras_.emplace(name,std::move(newCamera));

	// もしアクティブカメラがまだ設定されていなければ、作ったばかりのこれをアクティブにする
	if(activeCamera_ == nullptr){
		// get() で生ポインタを取得してセット (所有権は渡さない)
		activeCamera_ = cameras_[name].get();
	}
}

/// <summary>
/// アクティブカメラの切り替え
/// </summary>
void CameraManager::SetActiveCamera(const std::string& name){
	// 指定された名前のカメラが存在するか確認
	if(cameras_.contains(name)){
		// マップからポインタを取得してアクティブに設定
		activeCamera_ = cameras_.at(name).get();
	}
}

/// <summary>
/// 現在のアクティブカメラ取得
/// </summary>
Camera* CameraManager::GetActiveCamera() const{
	return activeCamera_;
}

/// <summary>
/// 名前指定でカメラ取得
/// </summary>
Camera* CameraManager::GetCamera(const std::string& name) const{
	if(cameras_.contains(name)){
		return cameras_.at(name).get();
	}
	return nullptr;
}