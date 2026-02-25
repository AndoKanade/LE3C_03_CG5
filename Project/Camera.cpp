#include "Camera.h"
#include "WinAPI.h"

// コンストラクタ
Camera::Camera()
	: transform({
		{1.0f, 1.0f, 1.0f},   // scale
		{0.0f, 0.0f, 0.0f},   // rotate
		{0.0f, 0.0f, -10.0f}  // translate (画面に映るように手前に引く)
		})
	,fov(0.45f)
	,aspectRatio(float(WinAPI::kClientWidth) / float(WinAPI::kCliantHeight))
	,nearClip(0.1f)
	,farClip(100.0f)
	,worldMatrix(MakeAffineMatrix(transform.scale,transform.rotate,transform.translate))
	,viewMatrix(Inverse(worldMatrix))
	,projectionMatrix(MakePerspectiveFovMatrix(fov,aspectRatio,nearClip,farClip))
	,viewProjectionMatrix(Multiply(viewMatrix,projectionMatrix)){}

// 更新処理
void Camera::Update(){
	// ワールド行列の計算
	worldMatrix = MakeAffineMatrix(transform.scale,transform.rotate,transform.translate);

	// ビュー行列の計算 (ワールド行列の逆行列)
	viewMatrix = Inverse(worldMatrix);

	// プロジェクション行列の計算
	projectionMatrix = MakePerspectiveFovMatrix(fov,aspectRatio,nearClip,farClip);

	// 合成行列の計算 (View * Projection)
	viewProjectionMatrix = Multiply(viewMatrix,projectionMatrix);
}