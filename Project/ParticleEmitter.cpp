#include "ParticleEmitter.h"
#include "ParticleManager.h" 

ParticleEmitter::ParticleEmitter(const std::string& name,const Transform& transform,uint32_t count,float frequency)
	: name_(name),transform_(transform),count_(count),frequency_(frequency){}

// ■ 追加: Emit関数の実装
void ParticleEmitter::Emit(){
	// エミッタの設定値に従って ParticleManager の Emit を呼び出すだけ
	ParticleManager::GetInstance()->Emit(name_,transform_.translate,count_);
}

void ParticleEmitter::Update(){
	timer_ += kDeltaTime_;

	// 発生頻度より大きいなら発生
	while(timer_ >= frequency_){

		// ■ 変更: 直接マネージャを呼ばず、自身の Emit() を使う
		this->Emit();

		// 余計に過ぎた時間も加味して頻度計算する
		timer_ -= frequency_;
	}
}