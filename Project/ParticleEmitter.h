#pragma once
#include "Math.h"
#include <string>

class ParticleEmitter{
public:
	// ■ コンストラクタでメンバ変数を初期化
	ParticleEmitter(const std::string& name,const Transform& transform,uint32_t count,float frequency);

	// 更新処理
	void Update();
	void Emit();

private:
	std::string name_;       // パーティクルグループ名
	Transform transform_;    // 発生位置
	uint32_t count_;         // 一度の発生数
	float frequency_;        // 発生頻度 (秒)

	float timer_ = 0.0f;     // 時間計測用
	const float kDeltaTime_ = 1.0f / 60.0f; // デルタタイム
};