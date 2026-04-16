#pragma once
#include "MyMath.h"
#include <string>
#include <vector>
#include <d3d12.h>
#include <wrl.h>
#include <memory> // ★追加: スマートポインタ用

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

	// ★追加: 親オブジェクトをセットする (循環参照防止のため weak_ptr)
	void SetParent(const std::weak_ptr<Obj3D>& parent);

	// モデルをセットする
	void SetModel(Model* model){ this->model = model; }
	void SetModel(const std::string& filePath);

	void SetCamera(Camera* camera){ this->camera = camera; }

	// Transformの各要素を設定
	void SetScale(const Vector3& scale){ transform.scale = scale; }
	void SetRotate(const Vector3& rotate){ transform.rotate = rotate; }
	void SetTranslate(const Vector3& translate){ transform.translate = translate; }
	void SetLightDirection(const Vector3& direction){
		if(directionalLightData){
			directionalLightData->direction = Normalize(direction);
		}
	}

	// --- Getter (値を取得する関数) ---

	// Transformの各要素を取得
	const Vector3& GetScale() const{ return transform.scale; }
	const Vector3& GetRotate() const{ return transform.rotate; }
	const Vector3& GetTranslate() const{ return transform.translate; }

	// ★追加: ワールド行列の取得
	// (子供が自分の位置を計算するときに、親のワールド行列が必要になるため)
	const Matrix4x4& GetWorldMatrix() const{
		return transformationMatrixData->World;
	}

private: // --- メンバ変数 ---

	// 共通リソースへのポインタ
	Obj3dCommon* object3dCommon = nullptr;

	// 表示するモデルへのポインタ
	Model* model = nullptr;

	Camera* camera = nullptr;

	// ★追加: 親オブジェクトへの参照
	// shared_ptr だと「親が子を持ち、子が親を持つ」とメモリが消えなくなる(循環参照)ので
	// 子供から親への参照は weak_ptr (弱い参照) にするのが鉄則です。
	std::weak_ptr<Obj3D> parent_;

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