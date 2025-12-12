#pragma once
#include <map>
#include <string>
#include <memory>
#include "Model.h"

// 前方宣言 (DXCommon.hをインクルードせずにポインタとして使うため)
class DXCommon;

class ModelManager{
public:
	// シングルトンインスタンスの取得
	static ModelManager* GetInstance();

	// 終了処理
	static void Finalize();

	// 初期化
	void Initialize(DXCommon* dxCommon);

	// モデルの読み込み (.objファイル名を指定)
	void LoadModel(const std::string& filePath);

	// モデルの検索 (読み込み済みのモデルを取得)
	Model* FindModel(const std::string& filePath);

private:
	// シングルトン管理用
	static ModelManager* instance;

	ModelManager() = default;
	~ModelManager() = default;
	ModelManager(const ModelManager&) = delete;
	ModelManager& operator=(const ModelManager&) = delete;

	// メンバ変数
	ModelCommon* modelCommon = nullptr;
	std::map<std::string,std::unique_ptr<Model>> models;
};