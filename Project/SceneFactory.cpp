#include "SceneFactory.h"
#include "TitleScene.h"
#include "GameScene.h"

// ★追加: make_unique を使うために必要
#include <memory>

// ★変更: 戻り値を BaseScene* から unique_ptr に修正
std::unique_ptr<BaseScene> SceneFactory::CreateScene(const std::string& sceneName){
	// 文字列を見て、対応するクラスを生成して返す
	std::unique_ptr<BaseScene> newScene = nullptr;

	if(sceneName == "TITLE"){
		// ★変更: new ではなく make_unique で生成
		newScene = std::make_unique<TitleScene>();
	} else if(sceneName == "GAME"){
		// ★変更: new ではなく make_unique で生成
		newScene = std::make_unique<GameScene>();
	}

	return newScene;
}