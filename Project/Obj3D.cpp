#include "Obj3D.h"
#include"Obj3dCommon.h"
#include<cassert>
#include<fstream>
#include"Math.h"
#include <iostream>
#include "Sprite.h"
#include "TextureManager.h"

void Obj3D::Initialize(Obj3dCommon* object3dCommon){
	this->object3dCommon = object3dCommon;

	modelData = LoadObjFile("resource","plane.obj");
	TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);
	modelData.material.textureIndex = TextureManager::GetInstance()->GetTextureIndexbyFilePath(modelData.material.textureFilePath);

	transform = {{1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f}};
	//cameraTransform = {
	//	{1.0f, 1.0f, 1.0f}, {0.3f, 0.0f, 0.0f}, {0.0f, 4.0f, -10.0f}};
	cameraTransform = {
	{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -10.0f}};

	CreateVertexData();
	CreateMaterialData();
	CreateTransformationMatrixData();
	CreateDirectionalLightData();

}

void Obj3D::Update(){
	// 1. ワールド行列 (自分の transform を使う)
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale,transform.rotate,transform.translate);

	// 2. カメラ行列 (★ここを修正: Initializeで設定した cameraTransform を使う)
	Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale,cameraTransform.rotate,cameraTransform.translate);
	Matrix4x4 viewMatrix = Inverse(cameraMatrix);

	// 3. プロジェクション行列
	Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f,1280.0f / 720.0f,0.1f,100.0f);

	// 4. 定数バッファ転送
	if(transformationMatrixData){
		Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix,Multiply(viewMatrix,projectionMatrix));
		transformationMatrixData->WVP = worldViewProjectionMatrix;
		transformationMatrixData->World = worldMatrix;
	}
}

void Obj3D::Draw(){

	// 1. 頂点バッファの設定
	object3dCommon->GetDxCommon()->commandList.Get()->IASetVertexBuffers(0,1,&vertexBufferView);

	// ※ トポロジの設定などはCommon側の共通設定で行われている前提です

	// 2. マテリアルCBufferの場所を設定 (RootParameter[0])
	object3dCommon->GetDxCommon()->commandList.Get()->SetGraphicsRootConstantBufferView(
		0,materialResource->GetGPUVirtualAddress());

	// 3. 座標変換行列CBufferの場所を設定 (RootParameter[1])
	object3dCommon->GetDxCommon()->commandList.Get()->SetGraphicsRootConstantBufferView(
		1,transformationMatrixResource->GetGPUVirtualAddress());

	// 4. SRV(テクスチャ)のDescriptorTableを設定 (RootParameter[2])
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandle =
		TextureManager::GetInstance()->GetSrvHandleGPU(modelData.material.textureIndex);

	object3dCommon->GetDxCommon()->commandList.Get()->SetGraphicsRootDescriptorTable(
		2,textureSrvHandle);

	// 5. 平行光源CBufferの場所を設定 (RootParameter[3])
	object3dCommon->GetDxCommon()->commandList.Get()->SetGraphicsRootConstantBufferView(
		3,directionalLightResource->GetGPUVirtualAddress());

	// 6. 描画コマンド (DrawInstanced)
	object3dCommon->GetDxCommon()->commandList.Get()->DrawInstanced(
		UINT(modelData.vertices.size()),1,0,0);
}
Obj3D::MaterialData Obj3D::LoadMaterialTemplateFile(const std::string& directoryPath,const std::string& filename){
	MaterialData materialData;
	std::string line;
	std::ifstream file(directoryPath + "/" + filename);

	assert(file.is_open());

	while(std::getline(file,line)){
		std::string identifier;
		std::istringstream s(line);

		s >> identifier;

		if(identifier == "map_Kd"){
			std::string textureFilename;
			s >> textureFilename;

			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}
	return materialData;
}

Obj3D::ModelData Obj3D::LoadObjFile(const std::string& directoryPath,const std::string& filename){
	ModelData modelData;
	std::vector<Vector4> positions;
	std::vector<Vector3> normals;
	std::vector<Vector2> texcoords;

	std::ifstream file(directoryPath + "/" + filename);
	if(!file.is_open()){
		std::cerr << "Failed to open OBJ file: " << directoryPath + "/" + filename
			<< std::endl;
		assert(false);
		return {};
	}

	std::string line;

	while(std::getline(file,line)){
		std::istringstream s(line);
		std::string identifier;
		s >> identifier;

		if(identifier == "v"){
			Vector4 position{};

			s >> position.x >> position.y >> position.z;

			position.w = 1.0f;
			//  position.x *= -1.0f;
			positions.push_back(position);

		} else if(identifier == "vt"){
			Vector2 texcoord{};
			s >> texcoord.x >> texcoord.y;

			texcoord.y = 1.0f - texcoord.y;
			texcoords.push_back(texcoord);

		} else if(identifier == "vn"){

			Vector3 normal{};

			s >> normal.x >> normal.y >> normal.z;
			//  normal.x *= -1.0f;
			normals.push_back(normal);

		} else if(identifier == "f"){

			VertexData triangle[3];

			for(int32_t faceVertex = 0; faceVertex < 3; ++faceVertex){
				std::string vdef;
				s >> vdef;

				std::istringstream vstream(vdef);
				uint32_t elementIndices[3];

				for(int32_t element = 0; element < 3; ++element){
					std::string index;
					std::getline(vstream,index,'/');
					elementIndices[element] = std::stoi(index);
				}

				Vector4 position = positions[elementIndices[0] - 1];
				Vector2 texcoord = texcoords[elementIndices[1] - 1];
				Vector3 normal = normals[elementIndices[2] - 1];

				triangle[faceVertex] = {position, texcoord, normal};
			}
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);

		} else if(identifier == "mtllib"){
			std::string materialFilename;
			s >> materialFilename;

			modelData.material =
				LoadMaterialTemplateFile(directoryPath,materialFilename);
		}
	}
	file.close();
	return modelData;
};

void Obj3D::CreateVertexData(){

	vertexResource = object3dCommon->GetDxCommon()->CreateBufferResource(
		sizeof(VertexData) * modelData.vertices.size());

	// VBV (Vertex Buffer View) の設定
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();

	//  uint32_t はインデックス用なので間違い。頂点1個分のサイズ × 個数
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());

	//  これが無いとGPUが「1頂点が何バイトか」分からないので必須
	vertexBufferView.StrideInBytes = sizeof(VertexData);


	//  データを書き込む (Mapしてコピー)
	// これをしないと、確保したメモリの中身が空っぽ（ゴミデータ）のままです
	VertexData* vertexDataPtr = nullptr;

	// GPUメモリをCPUから触れるようにする
	vertexResource->Map(0,nullptr,reinterpret_cast<void**>(&vertexDataPtr));

	// 用意していた modelData.vertices の中身をコピーする
	std::memcpy(vertexDataPtr,modelData.vertices.data(),sizeof(VertexData) * modelData.vertices.size());

	// 書き込み終わったら閉じる（UnmapはMapしっぱなしでも良い場合もあるが、基本は対にする）
	vertexResource->Unmap(0,nullptr);
}

void Obj3D::CreateMaterialData(){
	//  作ったリソースをメンバ変数の materialResource に「代入」する！
	materialResource = object3dCommon->GetDxCommon()->CreateBufferResource(sizeof(Material));

	//  メンバ変数の materialData にアドレスを入れる
	// (ローカル変数を新しく作るのではなく、クラスのメンバ変数を使います)
	// Sprite::Material ではなく、Obj3D内の Material 型を使う
	materialResource->Map(0,nullptr,reinterpret_cast<void**>(&materialData));

	//  データの初期化
	// いきなり代入してもいいですが、個別に設定する方が後で変更しやすいです
	materialData->color = Vector4(1.0f,1.0f,1.0f,1.0f);

	// ライティングを有効にする (int32_t型なら 1, boolなら true)
	materialData->enableLighting = 1; // 3Dなので通常はON(1)にする

	materialData->uvTransform = MakeIdentity4x4();
}

void Obj3D::CreateTransformationMatrixData(){
	// 1. 座標変換行列リソースを作る
	// 指定するサイズは sizeof(TransformationMatrix) です
	transformationMatrixResource = object3dCommon->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));

	// 2. リソースにデータを書き込むためのアドレスを取得して transformationMatrixData に割り当てる
	// これで transformationMatrixData->WVP などと書けるようになります
	transformationMatrixResource->Map(0,nullptr,reinterpret_cast<void**>(&transformationMatrixData));

	// 3. データの初期化 (単位行列を書き込んでおく)
	// 最初は変な変形をしないように、何もしない行列(Identity)を入れます
	transformationMatrixData->WVP = MakeIdentity4x4();
	transformationMatrixData->World = MakeIdentity4x4();
}

void Obj3D::CreateDirectionalLightData(){
	// 1. リソースを作成してメンバ変数に保存
	directionalLightResource = object3dCommon->GetDxCommon()->CreateBufferResource(sizeof(DirectionalLight));

	// 2. Mapしてポインタを取得
	directionalLightResource->Map(0,nullptr,reinterpret_cast<void**>(&directionalLightData));

	// 3. 初期値を設定 (main.cppの値と同じにする)
	directionalLightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
	directionalLightData->direction = {0.0f, 0.0f, -1.0f};
	directionalLightData->intensity = 1.0f;
}
