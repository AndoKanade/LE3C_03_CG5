#pragma once
#include "Framework.h"
#include "RenderTexture.h"
#include "PostProcess.h"
#include "SceneManager.h"
#include <memory>

/// <summary>
/// アプリケーションクラス
/// Frameworkクラスを継承し、ゲーム固有の処理を実装します。
/// </summary>
class Application : public Framework{
public:
	// --- コンストラクタ・デストラクタ ---
	Application();
	~Application() override;

	// --- メンバ関数 ---
	void Initialize() override; // 初期化処理
	void Finalize() override;   // 終了処理
	void Update() override;     // 更新処理
	void Draw() override;       // 描画処理

	// --- デバッグ・アクセサ ---
	static Application* GetInstance(){ return instance_; }
	void ShowPostProcessUI();

private:
	// 静的メンバ変数 (シングルトン用)
	static Application* instance_;

	// --- メンバリソース ---
	std::unique_ptr<RenderTexture> renderTexture_;
	std::unique_ptr<PostProcess> postProcess_;
};