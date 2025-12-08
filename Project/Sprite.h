#pragma once
#include"Math.h"
#include <wrl.h>
#include <d3d12.h>
#include<string>

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

	void Initialize(SpriteCommon* spriteCommon,std::string textureFilePath);
	void Update();
	void Draw();

	const Vector2& GetPosition() const{ return position; }
	void SetPosition(const Vector2& position){ this->position = position; }

	float GetRotation() const{ return rotation; }
	void SetRotation(float rotation){ this->rotation = rotation; }

	const Vector4& GetColor()const{ return materialData->color; }
	void SetColor(const Vector4& color){ materialData->color = color; }

	const Vector2& GetSize() const{ return size; }
	void SetSize(const Vector2& size){ this->size = size; }

	const Vector2& GetAnchorPoint() const{ return anchorPoint; }
	void SetAnchorPoint(const Vector2& anchorPoint){ this->anchorPoint = anchorPoint; }

	bool IsFlipX() const{ return isFlipX_; }
	void SetFlipX(bool isFlipX){ isFlipX_ = isFlipX; }

	bool IsFlipY() const{ return isFlipY_; }
	void SetFlipY(bool isFlipY){ isFlipY_ = isFlipY; }

	bool GetTextureLeftTop(Vector2* leftTop) const{
		if(!leftTop)return false;
		*leftTop = textureLeftTop;
		return true;
	}
	void SetTextureLeftTop(const Vector2& leftTop){
		textureLeftTop = leftTop;
	}
	bool GetTextureSize(Vector2* size) const{
		if(!size)return false;
		*size = textureSize;
		return true;
	}
	void SetTextureSize(const Vector2& size){
		textureSize = size;
	}

	SpriteCommon* spriteCommon = nullptr;

private:

	void CreateVertexData();
	void CreateMaterialData();
	void CreateTransformationMatrixData();

	Vector2 position = {0.0f,0.0f};
	float rotation = 0.0f;
	Vector2 size = {640.0f,360.0f};

	uint32_t textureIndex = 0;

	Vector2 anchorPoint = {0.0f,0.0f};
	bool isFlipX_ = false;
	bool isFlipY_ = false;

	Vector2 textureLeftTop = {0.0f,0.0f};
	Vector2 textureSize = {100.0f,100.0f};

	void AdjustTextureSize();
};

