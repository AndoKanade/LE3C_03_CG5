#include "Obj3D.h"
#include "Obj3dCommon.h"
#include "Model.h"
#include "TextureManager.h"
#include "Sprite.h"
#include "Math.h"
#include <cassert>
#include "ModelManager.h"

void Obj3D::Initialize(Obj3dCommon* object3dCommon){
	this->object3dCommon = object3dCommon;

	// トランスフォームの初期化
	transform = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

	// カメラの初期化
	cameraTransform = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -10.0f}};

	// リソース（バッファ）の生成
	CreateTransformationMatrixData();
	CreateDirectionalLightData();
}

void Obj3D::Update(){
	// 1. ワールド行列の計算 (Scale -> Rotate -> Translate)
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale,transform.rotate,transform.translate);

	// 2. カメラ行列 (View行列) の計算
	Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale,cameraTransform.rotate,cameraTransform.translate);
	Matrix4x4 viewMatrix = Inverse(cameraMatrix);

	// 3. プロジェクション行列の計算
	Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f,1280.0f / 720.0f,0.1f,100.0f);

	// 4. 定数バッファへの転送
	if(transformationMatrixData){
		// 行列を合成 (World * View * Projection)
		Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix,Multiply(viewMatrix,projectionMatrix));

		// GPU上のバッファに書き込み
		transformationMatrixData->WVP = worldViewProjectionMatrix;
		transformationMatrixData->World = worldMatrix;
	}
}

void Obj3D::Draw(){
	// コマンドリストを短く書けるように変数に入れる
	auto* commandList = object3dCommon->GetDxCommon()->commandList.Get();

	// 1. 座標変換行列CBufferの設定 (RootParameter[1])
	commandList->SetGraphicsRootConstantBufferView(
		1,transformationMatrixResource->GetGPUVirtualAddress());

	// 2. 平行光源CBufferの設定 (RootParameter[3])
	commandList->SetGraphicsRootConstantBufferView(
		3,directionalLightResource->GetGPUVirtualAddress());

	// 3. モデルの描画
	if(model){
		model->Draw();
	}
}

void Obj3D::SetModel(const std::string& filePath){
	model = ModelManager::GetInstance()->FindModel(filePath);
}

void Obj3D::CreateTransformationMatrixData(){
	// リソースを作成 (サイズ: TransformationMatrix構造体)
	transformationMatrixResource = object3dCommon->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));

	// データを書き込むためのアドレスを取得 (Map)
	transformationMatrixResource->Map(0,nullptr,reinterpret_cast<void**>(&transformationMatrixData));

	// 初期化: 単位行列を入れておく
	transformationMatrixData->WVP = MakeIdentity4x4();
	transformationMatrixData->World = MakeIdentity4x4();
}

void Obj3D::CreateDirectionalLightData(){
	// リソースを作成 (サイズ: DirectionalLight構造体)
	directionalLightResource = object3dCommon->GetDxCommon()->CreateBufferResource(sizeof(DirectionalLight));

	// データを書き込むためのアドレスを取得 (Map)
	directionalLightResource->Map(0,nullptr,reinterpret_cast<void**>(&directionalLightData));

	// 初期値の設定
	directionalLightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
	directionalLightData->direction = {0.0f, 0.0f, -1.0f};
	directionalLightData->intensity = 1.0f;
}