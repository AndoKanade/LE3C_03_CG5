#pragma once
#include "DXCommon.h"
class ModelCommon{
public:
	void Initialize(DXCommon* dxCommon);

	DXCommon* GetDxCommon()const{ return dxCommon_; }

private:
	DXCommon* dxCommon_;
};

