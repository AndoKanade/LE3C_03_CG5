#pragma once
#include "Math.h"
#include <string>
#include <vector>
#include <d3d12.h>
#include <wrl.h>

// 前方宣言
class Obj3dCommon;
class Model;
class Camera;

class Obj3D{
public:
	// 定数バッファ用データ構造体 (座標変換行列)
	struct TransformationMatrix{
		Matrix4x4 WVP;
		Matrix4x4 World;
	};

	// 定数バッファ用データ構造体 (平行光源)
	struct DirectionalLight{
		Vector4 color;     // ライトの色
		Vector3 direction; // ライトの向き
		float intensity;   // 輝度
	};

public: // --- メンバ関数 ---

	// 初期化
	void Initialize(Obj3dCommon* object3dCommon);

	// 更新
	void Update();

	// 描画
	void Draw();

	// --- Setter (値を設定する関数) ---

	// モデルをセットする
	void SetModel(Model* model){ this->model = model; }
	void SetModel(const std::string& filePath);

	void SetCamera(Camera* camera){ this->camera = camera; }

	// Transformの各要素を設定
	void SetScale(const Vector3& scale){ transform.scale = scale; }
	void SetRotate(const Vector3& rotate){ transform.rotate = rotate; }
	void SetTranslate(const Vector3& translate){ transform.translate = translate; }

	// --- Getter (値を取得する関数) ---

	// Transformの各要素を取得
	const Vector3& GetScale() const{ return transform.scale; }
	const Vector3& GetRotate() const{ return transform.rotate; }
	const Vector3& GetTranslate() const{ return transform.translate; }
	void SetLightDirection(const Vector3& direction){
		if(directionalLightData){
			directionalLightData->direction = Normalize(direction);
		}
	}
private: // --- メンバ変数 ---

	// 共通リソースへのポインタ
	Obj3dCommon* object3dCommon = nullptr;

	// 表示するモデルへのポインタ
	Model* model = nullptr;

	Camera* camera = nullptr;

	// 座標変換行列用リソース
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource;
	TransformationMatrix* transformationMatrixData = nullptr;

	// 平行光源用リソース
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource;
	DirectionalLight* directionalLightData = nullptr;

	// オブジェクトの変形情報
	Transform transform;

private: // --- プライベート関数 ---
	void CreateTransformationMatrixData();
	void CreateDirectionalLightData();
};