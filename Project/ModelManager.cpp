#include "ModelManager.h"

// staticメンバ変数の初期化
ModelManager* ModelManager::instance = nullptr;

ModelManager* ModelManager::GetInstance(){
	// インスタンスがなければ生成する
	if(instance == nullptr){
		instance = new ModelManager();
	}
	return instance;
}

void ModelManager::Finalize(){
	if(instance != nullptr){
		// ModelCommonの解放
		if(instance->modelCommon != nullptr){
			delete instance->modelCommon;
			instance->modelCommon = nullptr;
		}

		// インスタンス自体の解放
		delete instance;
		instance = nullptr;
	}
}

void ModelManager::Initialize(DXCommon* dxCommon){
	// モデル共通部の初期化
	modelCommon = new ModelCommon();
	modelCommon->Initialize(dxCommon);
}

void ModelManager::LoadModel(const std::string& filePath){
	// 読み込み済みなら何もしない
	if(models.contains(filePath)){
		return;
	}

	// モデルの生成と初期化
	std::unique_ptr<Model> model = std::make_unique<Model>();
	model->Initialize(modelCommon,"resource",filePath);

	// マップに登録（所有権を移動）
	models.insert(std::make_pair(filePath,std::move(model)));
}

Model* ModelManager::FindModel(const std::string& filePath){
	// 指定されたモデルがあればポインタを返す
	if(models.contains(filePath)){
		return models.at(filePath).get();
	}
	// なければnullptr
	return nullptr;
}