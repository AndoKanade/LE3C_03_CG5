#pragma once
#include "Framework.h"
#include "SceneManager.h"

/// <summary>
/// ゲームの実装を行うクラス
/// (Frameworkを継承し、シーンを管理する仲介役)
/// </summary>
class Application : public Framework{
public:
	// コンストラクタ・デストラクタ
	Application();
	~Application();

	// オーバーライド (Frameworkの関数を上書き)
	void Initialize() override;
	void Finalize() override;
	void Update() override;
	void Draw() override;

private:

	SceneManager* sceneManager_ = nullptr;
	SpriteCommon* spriteCommon_ = nullptr;

};