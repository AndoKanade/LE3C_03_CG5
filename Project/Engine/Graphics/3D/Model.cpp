#include "Model.h"
#include "TextureManager.h"
#include "SrvManager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cassert>

void Model::Initialize(ModelCommon* modelCommon,const std::string& directorypath,const std::string& filename){
	this->modelCommon_ = modelCommon;

	// モデルデータの読み込み (.obj)
	// TODO: 将来的には引数でファイル名を受け取るように変更する
	modelData = LoadObjFile(directorypath,filename);

	// テクスチャの読み込み (.mtlから取得したパスを使用)
	TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);

	// テクスチャ番号(Index)を取得して保存
	modelData.material.textureIndex =
		TextureManager::GetInstance()->GetSrvIndex(modelData.material.textureFilePath);

	// バッファの生成 (頂点・マテリアル)
	CreateVertexData();
	CreateMaterialData();
}

void Model::Draw(){
	// コマンドリストを取得
	auto* commandList = modelCommon_->GetDxCommon()->commandList.Get();

	// 1. 頂点バッファ(VBV)をセット
	commandList->IASetVertexBuffers(0,1,&vertexBufferView);

	// 2. マテリアルCBufferをセット (RootParameter[0])
	commandList->SetGraphicsRootConstantBufferView(
		0,materialResource->GetGPUVirtualAddress());

	// 3. テクスチャSRVをセット (RootParameter[2])
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandle =
		SrvManager::GetInstance()->GetGPUDescriptorHandle(modelData.material.textureIndex);
	commandList->SetGraphicsRootDescriptorTable(2,textureSrvHandle);

	// 4. 描画コマンド発行
	commandList->DrawInstanced(UINT(modelData.vertices.size()),1,0,0);
}

Model::MaterialData Model::LoadMaterialTemplateFile(const std::string& directoryPath,const std::string& filename){
	MaterialData materialData;
	std::string line;
	std::ifstream file(directoryPath + "/" + filename);

	assert(file.is_open());

	while(std::getline(file,line)){
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		// "map_Kd": テクスチャファイル名
		if(identifier == "map_Kd"){
			std::string textureFilename;
			s >> textureFilename;
			// パスを結合して保存
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}
	return materialData;
}

Model::ModelData Model::LoadObjFile(const std::string& directoryPath,const std::string& filename){
	ModelData modelData;

	// 一時データ用
	std::vector<Vector4> positions;
	std::vector<Vector3> normals;
	std::vector<Vector2> texcoords;

	std::ifstream file(directoryPath + "/" + filename);
	if(!file.is_open()){
		std::cerr << "Failed to open OBJ file: " << directoryPath + "/" + filename << std::endl;
		assert(false);
		return {};
	}

	std::string line;
	while(std::getline(file,line)){
		std::istringstream s(line);
		std::string identifier;
		s >> identifier;

		// 頂点位置 (v)
		if(identifier == "v"){
			Vector4 position{};
			s >> position.x >> position.y >> position.z;
			position.w = 1.0f;
			positions.push_back(position);
		}
		// テクスチャ座標 (vt)
		else if(identifier == "vt"){
			Vector2 texcoord{};
			s >> texcoord.x >> texcoord.y;
			texcoord.y = 1.0f - texcoord.y; // Y反転
			texcoords.push_back(texcoord);
		}
		// 法線 (vn)
		else if(identifier == "vn"){
			Vector3 normal{};
			s >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		}
		// 面情報 (f)
		else if(identifier == "f"){
			// まず3つの頂点を読み込む (三角形の1つ目)
			VertexData triangle[3];
			for(int32_t faceVertex = 0; faceVertex < 3; ++faceVertex){
				std::string vdef;
				s >> vdef;

				// "頂点/UV/法線" を分解
				std::istringstream vstream(vdef);
				uint32_t elementIndices[3];
				for(int32_t element = 0; element < 3; ++element){
					std::string index;
					std::getline(vstream,index,'/');
					elementIndices[element] = index.empty()?0:std::stoi(index);
				}

				// インデックス取得 (OBJは1始まりなので-1する)
				Vector4 position = positions[elementIndices[0] - 1];
				Vector2 texcoord = texcoords[elementIndices[1] - 1];
				Vector3 normal = normals[elementIndices[2] - 1];
				triangle[faceVertex] = {position, texcoord, normal};
			}

			// 1つ目の三角形を登録 (逆順登録: 2 -> 1 -> 0)
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);

			std::string vdef4;
			if(s >> vdef4){
				// 4つ目があったので解析する
				std::istringstream vstream(vdef4);
				uint32_t elementIndices[3];
				for(int32_t element = 0; element < 3; ++element){
					std::string index;
					std::getline(vstream,index,'/');
					elementIndices[element] = index.empty()?0:std::stoi(index);
				}

				Vector4 position = positions[elementIndices[0] - 1];
				Vector2 texcoord = texcoords[elementIndices[1] - 1];
				Vector3 normal = normals[elementIndices[2] - 1];
				VertexData v4 = {position, texcoord, normal};

				// 2つ目の三角形を登録 (四角形を分割する)
				// 構成: 元の[0], [2] と 新しい[3] を結ぶ
				// 逆順登録なので [3] -> [2] -> [0] の順でプッシュ
				modelData.vertices.push_back(v4);          // v3 (4番目)
				modelData.vertices.push_back(triangle[2]); // v2 (3番目)
				modelData.vertices.push_back(triangle[0]); // v0 (1番目)
			}
		}
		// マテリアル読み込み (mtllib)
		else if(identifier == "mtllib"){
			std::string materialFilename;
			s >> materialFilename;
			modelData.material = LoadMaterialTemplateFile(directoryPath,materialFilename);
		}
	}
	file.close();
	return modelData;
}

void Model::CreateVertexData(){
	// リソース作成
	vertexResource = modelCommon_->GetDxCommon()->CreateBufferResource(
		sizeof(VertexData) * modelData.vertices.size());

	// VBVの設定
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	// データの書き込み (Map -> memcpy -> Unmap)
	VertexData* vertexDataPtr = nullptr;
	vertexResource->Map(0,nullptr,reinterpret_cast<void**>(&vertexDataPtr));
	std::memcpy(vertexDataPtr,modelData.vertices.data(),sizeof(VertexData) * modelData.vertices.size());
	vertexResource->Unmap(0,nullptr);
}

void Model::CreateMaterialData(){
	// ■■■ 1. バッファサイズの計算 (256バイトアライメント) ■■■
	// これをしないと、GPUが「データが足りない！(範囲外アクセス)」とエラーを出します
	size_t sizeInBytes = (sizeof(Material) + 0xff) & ~0xff;

	// リソース作成 (計算した sizeInBytes を使う)
	materialResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeInBytes);

	// データを書き込むためのアドレスを取得
	materialResource->Map(0,nullptr,reinterpret_cast<void**>(&materialData));

	// 初期値設定
	materialData->color = Vector4(1.0f,1.0f,1.0f,1.0f);
	materialData->enableLighting = 1;
	materialData->uvTransform = MakeIdentity4x4();

	// ■■■ 2. shininess の初期化を追加 ■■■
	materialData->shininess = 50.0f; // 輝きの鋭さ (とりあえず50.0f)
}