#include "ModelManager.h"
#include "DXCommon.h" // Initializeで使うため必要ならインクルード
#include "ModelCommon.h" // ★追加: ModelCommonの実体が必要

// ★削除: staticメンバ変数の初期化は不要
// ModelManager* ModelManager::instance = nullptr;

ModelManager* ModelManager::GetInstance(){
	// ★変更: Meyers Singleton (staticローカル変数)
	// これで new も delete も不要になります。
	static ModelManager instance;
	return &instance;
}

void ModelManager::Finalize(){
	// ★変更: インスタンスの削除は自動で行われるので不要です。
	// ここでは保持しているモデルデータをクリアするだけでOK。
	models.clear();

	// modelCommon も unique_ptr なので、
	// ModelManager が死ぬときに勝手に道連れで消えてくれます。
}

void ModelManager::Initialize(DXCommon* dxCommon){
	// ★変更: make_unique で生成
	modelCommon = std::make_unique<ModelCommon>();
	modelCommon->Initialize(dxCommon);
}

void ModelManager::LoadModel(const std::string& filePath){
	// 読み込み済みなら何もしない
	// (C++20なら contains が使えますが、なければ find != end で代用)
	if(models.contains(filePath)){
		return;
	}

	// モデルの生成と初期化
	// (ここですでに make_unique を使えているのは素晴らしいです！)
	std::unique_ptr<Model> model = std::make_unique<Model>();

	// modelCommon は unique_ptr なので .get() で生ポインタを渡す
	model->Initialize(modelCommon.get(),"resource",filePath);

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