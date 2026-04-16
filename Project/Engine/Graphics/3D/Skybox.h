#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include "MyMath.h"
#include "Camera.h"

class SkyboxCommon;

class Skybox{
public:
	struct Vertex{
		Vector4 pos;
	};

	struct SkyboxMaterial{
		Vector4 color;
	};

	struct SkyboxTransformationMatrix{
		Matrix4x4 WVP;
	};

	void Initialize(SkyboxCommon* skyboxCommon,const std::string& texturePath);
	void Update(const Camera& camera);
	void Draw();

private:
	void CreateMesh();

private:
	SkyboxCommon* skyboxCommon_ = nullptr;

	// メッシュ関連
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

	// テクスチャ関連
	uint32_t srvIndex_ = 0;

	// 定数バッファ関連
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	SkyboxMaterial* materialData_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;
	SkyboxTransformationMatrix* wvpData_ = nullptr;
};