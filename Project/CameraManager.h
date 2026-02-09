#pragma once
#include <map>
#include <string>
#include <memory>
#include "Camera.h"

class CameraManager{
public:
	// シングルトンインスタンス取得
	static CameraManager* GetInstance();

	// 終了処理
	// ★変更: staticを外す (インスタンス内部のデータをクリアするため)
	void Finalize();

	// 初期化
	void Initialize();

	// 更新処理
	void Update();

	// --- カメラ操作関数 ---

	void CreateCamera(const std::string& name);
	void SetActiveCamera(const std::string& name);
	Camera* GetActiveCamera() const;
	Camera* GetCamera(const std::string& name);

private:
	// ★削除: static CameraManager* instance; は不要

	CameraManager() = default;
	~CameraManager() = default;
	CameraManager(const CameraManager&) = delete;
	CameraManager& operator=(const CameraManager&) = delete;

	// カメラのコンテナ (ここはunique_ptrですでにOK！)
	std::map<std::string,std::unique_ptr<Camera>> cameras;

	// 現在使用中のカメラ (所有権は持たないので生ポインタでOK)
	Camera* activeCamera = nullptr;
};