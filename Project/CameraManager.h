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
	static void Finalize();

	// 初期化
	void Initialize();

	// 更新処理 (アクティブなカメラを更新する)
	void Update();

	// --- カメラ操作関数 ---

	// 新しいカメラを作成する
	void CreateCamera(const std::string& name);

	// アクティブにするカメラを名前で指定する
	void SetActiveCamera(const std::string& name);

	// 現在アクティブなカメラを取得する
	Camera* GetActiveCamera() const;

	// 特定のカメラを取得する (調整用など)
	Camera* GetCamera(const std::string& name);

private:
	static CameraManager* instance;

	CameraManager() = default;
	~CameraManager() = default;
	CameraManager(const CameraManager&) = delete;
	CameraManager& operator=(const CameraManager&) = delete;

	// カメラのコンテナ
	std::map<std::string,std::unique_ptr<Camera>> cameras;

	// 現在使用中のカメラへのポインタ (コンテナ内のどれかを指す)
	Camera* activeCamera = nullptr;
};