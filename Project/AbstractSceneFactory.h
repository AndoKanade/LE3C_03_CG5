#pragma once
#include "BaseScene.h"
#include <string>
#include <memory> // ★追加: unique_ptrを使うために必須

/// <summary>
/// シーン工場のインターフェース (設計図)
/// </summary>
class AbstractSceneFactory{
public:
    virtual ~AbstractSceneFactory() = default;

    // シーン生成関数 (文字列を受け取って、そのシーンを返す)
    // ★変更: 戻り値を unique_ptr にする
    virtual std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) = 0;
};