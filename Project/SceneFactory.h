#pragma once
#include "AbstractSceneFactory.h"

class SceneFactory : public AbstractSceneFactory{
public:
    virtual ~SceneFactory() = default;

    // 親の関数をオーバーライドして実装
    BaseScene* CreateScene(const std::string& sceneName) override;
};