#include "Obj3D.h"
#include "Obj3dCommon.h"
#include "Model.h"
#include "TextureManager.h"
#include "Sprite.h"
#include "MyMath.h"
#include <cassert>
#include "ModelManager.h"
#include "Camera.h"

void Obj3D::Initialize(Obj3dCommon* object3dCommon){
	this->object3dCommon = object3dCommon;
	this->camera = object3dCommon->GetDefaultCamera();

	// トランスフォームの初期化
	transform = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

	// リソース（バッファ）の生成
	CreateTransformationMatrixData();
	CreateDirectionalLightData();
}

// ★追加: 親をセットする関数
void Obj3D::SetParent(const std::weak_ptr<Obj3D>& parent){
	this->parent_ = parent;
}

void Obj3D::Update(){
	// 1. ローカル行列の計算 (Scale -> Rotate -> Translate)
	// 親から見た自分自身の行列です
	Matrix4x4 localMatrix = MakeAffineMatrix(transform.scale,transform.rotate,transform.translate);

	// 2. ワールド行列の初期化
	Matrix4x4 worldMatrix = localMatrix;

	// ★ここが重要: 親がいるかどうかの確認
	// weak_ptr は直接使えないので、lock() して shared_ptr に変換してみる
	std::shared_ptr<Obj3D> parentPtr = parent_.lock();

	if(parentPtr){
		// lock() が成功した = 親が生きていた！
		// 親のワールド行列を取得して、自分のローカル行列に掛け合わせる
		// 行列の計算順序: World = Local * ParentWorld
		worldMatrix = Multiply(localMatrix,parentPtr->GetWorldMatrix());
	}
	// lock() が失敗した(nullptr) = 親がいない、または親が削除済み
	// その場合は worldMatrix = localMatrix のまま進むのでエラーにならない！


	// 3. カメラ行列との合成 (WVP)
	Matrix4x4 worldViewProjectionMatrix;
	// メンバ変数の camera を優先し、なければデフォルトカメラを使う
	Camera* cameraPtr = this->camera?this->camera:object3dCommon->GetDefaultCamera();

	if(cameraPtr){
		const Matrix4x4& viewProjectionMatrix = cameraPtr->GetViewProjectionMatrix();
		worldViewProjectionMatrix = Multiply(worldMatrix,viewProjectionMatrix);
	} else{
		worldViewProjectionMatrix = worldMatrix;
	}

	// 4. GPU上のバッファに書き込み
	transformationMatrixData->WVP = worldViewProjectionMatrix;
	transformationMatrixData->World = worldMatrix; // 親子関係が反映されたワールド行列を書き込む
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