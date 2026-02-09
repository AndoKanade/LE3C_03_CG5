#pragma once
#include "AbstractSceneFactory.h"
#include <memory> // ★追加: unique_ptrを使うために必須

class SceneFactory : public AbstractSceneFactory{
public:
    virtual ~SceneFactory() = default;

    // 親の関数をオーバーライドして実装
    // ★変更: 戻り値を unique_ptr にする (親クラスと合わせる必須作業)
    std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) override;
};