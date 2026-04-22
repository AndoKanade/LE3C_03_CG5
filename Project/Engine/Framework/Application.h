#pragma once
#include "Framework.h"
#include "RenderTexture.h"
#include "PostProcess.h"
#include "SceneManager.h"

/// <summary>
/// アプリケーションクラス
/// Frameworkクラスを継承し、ゲーム固有の処理を実装します。
/// </summary>
class Application : public Framework{
public:
	// --- コンストラクタ・デストラクタ ---
	Application();
	~Application() override;

	// --- 基底クラスの純粋仮想関数をオーバーライド ---

	// 初期化処理
	void Initialize() override;

	// 終了処理
	void Finalize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

private:
	// --- メンバ変数 ---
	std::unique_ptr<RenderTexture> renderTexture_;
	std::unique_ptr<PostProcess> postProcess_;
};