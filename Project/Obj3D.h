#pragma once
#include <string>
#include <vector> // ★std::vectorを使うために必要
#include <d3d12.h> // ★DirectXの型を使うために必要
#include <wrl.h>   // ★ComPtrを使うために必要
#include "Math.h"  // ★Vector3などを使うために必要

// 前方宣言
class Obj3dCommon;

class Obj3D{
public:

	struct VertexData{
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal;
	};

	struct MaterialData{
		std::string textureFilePath;
		uint32_t textureIndex = 0;
	};

	struct ModelData{
		std::vector<VertexData> vertices;
		MaterialData material;
	};

	// 定数バッファ用データ構造体
	struct Material{
		Vector4 color;
		int32_t enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
	};

	struct TransformationMatrix{
		Matrix4x4 WVP;
		Matrix4x4 World;
	};

	struct DirectionalLight{
		Vector4 color;     // ライトの色
		Vector3 direction; // ライトの方向
		float intensity;   // ライトの強度
	};


	// 初期化
	void Initialize(Obj3dCommon* object3dCommon);

	 void Update();
	 void Draw();

	// 静的関数 (モデル読み込み)
	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath,const std::string& filename);
	static ModelData LoadObjFile(const std::string& directoryPath,const std::string& filename);

private:

	Obj3dCommon* object3dCommon = nullptr;

	// モデルデータ
	ModelData modelData;

	// 頂点バッファ関連
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	VertexData* vertexData = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

	// マテリアル関連
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	Material* materialData = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource;
	TransformationMatrix* transformationMatrixData = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource;
	DirectionalLight* directionalLightData = nullptr;

	Transform transform;
	Transform cameraTransform;

	// 内部処理関数
	void CreateVertexData();
	void CreateMaterialData();
	void CreateTransformationMatrixData();
	void CreateDirectionalLightData();

};