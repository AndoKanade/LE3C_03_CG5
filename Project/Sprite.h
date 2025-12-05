#pragma once
#include"Math.h"
#include <wrl.h>
#include <d3d12.h>


class SpriteCommon;
class Sprite{
public:

	struct VertexData{
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal;

		bool operator<(const VertexData& other) const{
			if(position != other.position)
				return position < other.position;
			if(texcoord != other.texcoord)
				return texcoord < other.texcoord;
			if(normal != other.normal)
				return normal < other.normal;
			return false;
		}

	};

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

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;

	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;

	VertexData* vertexData = nullptr;
	uint32_t* indexData = nullptr;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;


	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	Material* materialData = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource;
	TransformationMatrix* transformationMatrixData = nullptr;

	Matrix4x4 worldMatrix;
	Matrix4x4 viewMatrix;
	Matrix4x4 projectionMatrix;

	void Initialize(SpriteCommon* spriteCommon);
	void Update();
	void Draw();

	SpriteCommon* spriteCommon = nullptr;

private:

	void CreateVertexData();
	void CreateMaterialData();
	void CreateTransformationMatrixData();

};

