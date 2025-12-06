#include "Sprite.h"
#include "SpriteCommon.h"
#include "TextureManager.h"

void Sprite::Initialize(SpriteCommon* spriteCommon,std::string textureFilePath){
	this->spriteCommon = spriteCommon;
	CreateVertexData();
	CreateMaterialData();
	CreateTransformationMatrixData();
	textureIndex = TextureManager::GetInstance()->GetTextureIndexbyFilePath(textureFilePath);
}

void Sprite::Update(){

	vertexResource->Map(0,nullptr,
		reinterpret_cast<void**>(&vertexData));

	//  矩形の4頂点（左上 → 左下 → 右上 → 右下）
	vertexData[0].position = {0.0f, 0.0f, 0.0f, 1.0f}; // 左上
	vertexData[0].texcoord = {0.0f, 0.0f};
	vertexData[0].normal = {0.0f, 0.0f, -1.0f};

	vertexData[1].position = {0.0f, 1.0f, 0.0f, 1.0f}; // 左下
	vertexData[1].texcoord = {0.0f, 1.0f};
	vertexData[1].normal = {0.0f, 0.0f, -1.0f};

	vertexData[2].position = {1.0f, 0.0f, 0.0f, 1.0f}; // 右上
	vertexData[2].texcoord = {1.0f, 0.0f};
	vertexData[2].normal = {0.0f, 0.0f, -1.0f};

	vertexData[3].position = {1.0f, 1.0f, 0.0f, 1.0f}; // 右下
	vertexData[3].texcoord = {1.0f, 1.0f};
	vertexData[3].normal = {0.0f, 0.0f, -1.0f};


	indexResource->Map(0,nullptr,
		reinterpret_cast<void**>(&indexData));

	// 三角形1: 左上 → 左下 → 右上
	indexData[0] = 0;
	indexData[1] = 2;
	indexData[2] = 1;

	//  三角形2: 左下 → 右下 → 右上
	indexData[3] = 1;
	indexData[4] = 2;
	indexData[5] = 3;


	/// Sprite用のWorldViewProjectionMatrixを作る

	Transform transform{
	{1.0f, 1.0f, 1.0f},
	{0.0f, 0.0f, 0.0f},
	{0.0f, 0.0f, 0.0f},

	};

	transform.translate = {position.x,position.y,0.0f};
	transform.rotate = {0.0f,0.0f,rotation};
	transform.scale = {size.x,size.y,1.0f};


	worldMatrix =
		MakeAffineMatrix(transform.scale,transform.rotate,
			transform.translate);
	viewMatrix = MakeIdentity4x4();
	projectionMatrix =
		MakeOrthographicMatrix(0.0f,0.0f,float(WinAPI::kCliantWidth),
			float(WinAPI::kCliantHeight),0.0f,100.0f);

	transformationMatrixData->WVP = Multiply(worldMatrix,Multiply(viewMatrix,projectionMatrix));
	transformationMatrixData->World = worldMatrix;

}

void Sprite::Draw(){
	spriteCommon->GetDxCommon()->commandList.Get()->IASetVertexBuffers(0,1,
		&vertexBufferView);
	spriteCommon->GetDxCommon()->commandList.Get()->IASetIndexBuffer(&indexBufferView);


	spriteCommon->GetDxCommon()->commandList.Get()->SetGraphicsRootConstantBufferView(
		0,materialResource->GetGPUVirtualAddress());
	spriteCommon->GetDxCommon()->commandList.Get()->SetGraphicsRootConstantBufferView(
		1,transformationMatrixResource->GetGPUVirtualAddress());
	spriteCommon->GetDxCommon()->commandList.Get()->SetGraphicsRootDescriptorTable(
		2,TextureManager::GetInstance()->GetSrvHandleGPU(textureIndex));

	spriteCommon->GetDxCommon()->commandList->DrawIndexedInstanced(6,1,0,0,0);


}

void Sprite::CreateVertexData(){

	vertexResource =
		spriteCommon->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * 6);

	indexResource =
		spriteCommon->GetDxCommon()->CreateBufferResource(sizeof(uint32_t) * 6);

	vertexBufferView.BufferLocation =
		vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = sizeof(VertexData) * 6;
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	indexBufferView.BufferLocation =
		indexResource->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes = sizeof(uint32_t) * 6;
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
}

void Sprite::CreateMaterialData(){

	materialResource =
		spriteCommon->GetDxCommon()->CreateBufferResource(sizeof(Material));
	materialResource->Map(0,nullptr,
		reinterpret_cast<void**>(&materialData));

	materialData->color = Vector4(1.0f,1.0f,1.0f,1.0f);

	materialData->enableLighting = false;
	materialData->uvTransform = MakeIdentity4x4();

}

void Sprite::CreateTransformationMatrixData(){

	transformationMatrixResource = spriteCommon->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));

	transformationMatrixResource->Map(
		0,nullptr,reinterpret_cast<void**>(&transformationMatrixData));
	transformationMatrixData->WVP = MakeIdentity4x4();
	transformationMatrixData->World = MakeIdentity4x4();

}
