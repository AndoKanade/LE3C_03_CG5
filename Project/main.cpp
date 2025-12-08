#define _USE_MATH_DEFINES
#define PI 3.14159265f
#include "DXCommon.h"
#include "Input.h"
#include "StringUtility.h"
#include "WinAPI.h"
#include <chrono>
#include <cmath>
#include <dbghelp.h>
#include <dxcapi.h>
#include <dxgidebug.h>
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>
#include <string.h>
#include <strsafe.h>
#include <vector>
#include <xaudio2.h>
#include"SpriteCommon.h"
#include "Sprite.h"
#include"Math.h"

#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include <iostream>
#include <map>
#include "TextureManager.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lPalam);

#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

using namespace DirectX;

LRESULT CALLBACK WindowProc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam){
	if(ImGui_ImplWin32_WndProcHandler(hwnd,msg,wparam,lparam)){
		return true;
	}

	switch(msg){
	case WM_DESTROY:

		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(hwnd,msg,wparam,lparam);
}

struct DirectionalLight{
	Vector4 color;     // ライトの色
	Vector3 direction; // ライトの方向
	float intensity;   // ライトの強度
};

struct MaterialData{
	std::string textureFilePath;
};

struct ModelData{
	std::vector<Sprite::VertexData> vertices;
	MaterialData material;
};

struct D3DResourceLeakChecker{
	~D3DResourceLeakChecker(){

		Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
		if(SUCCEEDED(DXGIGetDebugInterface1(0,IID_PPV_ARGS(&debug)))){
			debug->ReportLiveObjects(DXGI_DEBUG_ALL,DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP,DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12,DXGI_DEBUG_RLO_ALL);
		}
	}
};

#pragma region サウンド再生
struct ChunkHeader{
	char id[4];
	int32_t size;
};

struct RiffHeader{
	ChunkHeader chunk;
	char type[4];
};

struct FormatChunk{
	ChunkHeader chunk;
	WAVEFORMATEX fmt;
};

struct SoundData{
	WAVEFORMATEX wfex;
	BYTE* pBUffer;
	unsigned int bufferSize;
	;
};

#pragma endregion

struct WindowData{
	HINSTANCE hInstance;
	HWND hwnd;
	// 他にも必要な情報を含む
};

#pragma endregion

#pragma region 関数たち

#pragma region 音声データの読み込み

SoundData SoundLoadWave(const char* filename){

	std::ifstream file;
	file.open(filename,std::ios_base::binary);
	assert(file.is_open());

	RiffHeader riff;
	file.read((char*)&riff,sizeof(riff));

	if(strncmp(riff.chunk.id,"RIFF",4) != 0){
		assert(0);
	}

	if(strncmp(riff.type,"WAVE",4) != 0){
		assert(0);
	}

	FormatChunk format = {};
	file.read((char*)&format,sizeof(ChunkHeader));

	if(strncmp(format.chunk.id,"fmt ",4) != 0){
		assert(0);
	}

	assert(format.chunk.size <= sizeof(format.fmt));
	file.read((char*)&format.fmt,format.chunk.size);

	ChunkHeader data;
	file.read((char*)&data,sizeof(data));

	if(strncmp(data.id,"JUNK",4) == 0){
		file.seekg(data.size,std::ios_base::cur);

		file.read((char*)&data,sizeof(data));
	}

	if(strncmp(data.id,"data",4) != 0){
		assert(0);
	}

	char* pBuffer = new char[data.size];
	file.read(pBuffer,data.size);

	file.close();

	SoundData soundData = {};

	soundData.wfex = format.fmt;
	soundData.pBUffer = reinterpret_cast<BYTE*>(pBuffer);
	soundData.bufferSize = data.size;

	return soundData;
}

#pragma endregion

#pragma region 音声データの解放
void SoundUnload(SoundData* soundData){
	delete[] soundData->pBUffer;

	soundData->pBUffer = 0;
	soundData->bufferSize = 0;
	soundData->wfex = {};
}

#pragma endregion

#pragma region サウンドの再生

void SoundPlayWave(IXAudio2* xAudio2,const SoundData& soundData){
	HRESULT result;

	IXAudio2SourceVoice* pSourceVoice = nullptr;
	result = xAudio2->CreateSourceVoice(&pSourceVoice,&soundData.wfex);
	assert(SUCCEEDED(result));

	XAUDIO2_BUFFER buf{};

	buf.pAudioData = soundData.pBUffer;
	buf.AudioBytes = soundData.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;

	result = pSourceVoice->SubmitSourceBuffer(&buf);
	result = pSourceVoice->Start();
}

#pragma endregion

#pragma region log関数

// ログファイルの方
void Log(std::ostream& os,const std::string& message){
	os << message << std::endl;
	OutputDebugStringA(message.c_str());
}

//
void Log(const std::string& message){ OutputDebugStringA(message.c_str()); }
#pragma endregion

#pragma region ダンプ
static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception){

	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = {0};
	CreateDirectory(L"./Dumps",nullptr);
	StringCchPrintfW(filePath,MAX_PATH,L"./Dumps/%04d-%02d%02d-%02d%02d.dmp",
		time.wYear,time.wMonth,time.wDay,time.wHour,
		time.wMinute);
	HANDLE dumpFileHandle =
		CreateFile(filePath,GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_WRITE | FILE_SHARE_READ,0,CREATE_ALWAYS,0,0);

	// processIdとクラッシュの発生したthreadIdを取得
	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();

	// 設定情報を入力
	MINIDUMP_EXCEPTION_INFORMATION minidumpinformation{0};
	minidumpinformation.ThreadId = threadId;
	minidumpinformation.ExceptionPointers = exception;
	minidumpinformation.ClientPointers = TRUE;

	// Dumpを出力、MiniDumpWriteNormalは最低限の情報を出力するフラグ
	MiniDumpWriteDump(GetCurrentProcess(),processId,dumpFileHandle,
		MiniDumpNormal,&minidumpinformation,nullptr,nullptr);

	return EXCEPTION_EXECUTE_HANDLER;
}
#pragma endregion

#pragma region MaterialTemplate関数
MaterialData LoadMaterialTemplateFile(const std::string& directoryPath,
	const std::string& filename){
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
#pragma endregion

#pragma region Objファイルを読む関数

ModelData LoadObjFile(const std::string& directoryPath,
	const std::string& filename){
	ModelData modelData;
	std::vector<Vector4> positions;
	std::vector<Vector3> normals;
	std::vector<Vector2> texcoords;

	std::ifstream file(directoryPath + "/" + filename);
	if(!file.is_open()){
		std::cerr << "Failed to open OBJ file: " << directoryPath + "/" + filename
			<< std::endl;
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

			Sprite::VertexData triangle[3] = {};

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
}

#pragma endregion


#pragma endregion

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE,LPSTR,int){
	D3DResourceLeakChecker leakCheck;

	WinAPI* winApi = nullptr;

	Input* input = nullptr;

	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
	IXAudio2MasteringVoice* masterVoice;

	// 例外が発生したらダンプを出力する
	SetUnhandledExceptionFilter(ExportDump);

	winApi = new WinAPI();
	winApi->Initialize();

	DXCommon* dxCommon = nullptr;
	dxCommon = new DXCommon();
	dxCommon->Initialize(winApi);

	//	HRESULT hr;

	input = new Input();
	input->Initialize(winApi);

	TextureManager::GetInstance()->Initialize(dxCommon);
	TextureManager::GetInstance()->LoadTexture("resource/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("resource/monsterball.png");

	SpriteCommon* spriteCommon = nullptr;

	spriteCommon = new SpriteCommon;
	spriteCommon->Initialize(dxCommon);

	Sprite* sprite = new Sprite();
	sprite->Initialize(spriteCommon,"resource/uvChecker.png");

	Sprite* spriteBall = new Sprite();
	spriteBall->Initialize(spriteCommon,"resource/monsterball.png");
	spriteBall->SetPosition({200.0f,0.0f});


	SoundData soundData = SoundLoadWave("resource/You_and_Me.wav");
	bool hasPlayed = false;



#pragma endregion

#pragma region VertexResourceを生成する

	// 分割数（自由に調整可能）
	const int kLatitudeDiv = 16;  // 縦（経度）
	const int kLongitudeDiv = 16; // 横（緯度）

	int sphereVertexCount = kLatitudeDiv * kLongitudeDiv * 6;

	ModelData modelData = LoadObjFile("resource","plane.obj");

	// VertexResource を生成
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource =
		dxCommon->CreateBufferResource(sizeof(Sprite::VertexData) *
			modelData.vertices.size());

#pragma endregion

#pragma region IndexResourceを生成する

	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource =
		dxCommon->CreateBufferResource(sizeof(uint32_t) *
			modelData.vertices.size());

	D3D12_INDEX_BUFFER_VIEW indexBufferView{};
	indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes =
		UINT(sizeof(uint32_t) * modelData.vertices.size());
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

#pragma endregion

#pragma region MaterialResource

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource =
		dxCommon->CreateBufferResource(sizeof(Sprite::Material));
	Sprite::Material* materialData = nullptr;

	materialResource->Map(0,nullptr,reinterpret_cast<void**>(&materialData));
	*materialData = Sprite::Material{Vector4(1.0f, 1.0f, 1.0f, 1.0f), 1};
	materialData->enableLighting = true;
	materialData->uvTransform = MakeIdentity4x4();

#pragma endregion

#pragma region directionalLight

	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource =
		dxCommon->CreateBufferResource(sizeof(DirectionalLight));
	DirectionalLight* directionalLightData = nullptr;
	directionalLightResource->Map(
		0,nullptr,reinterpret_cast<void**>(&directionalLightData));

	directionalLightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
	directionalLightData->direction = {0.0f, 0.0f, -1.0f};
	directionalLightData->intensity = 1.0f;

#pragma endregion

#pragma region TransformationMatrix用のResourceを作る

	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource =
		dxCommon->CreateBufferResource(sizeof(TransformationMatrix));
	TransformationMatrix* wvpData = nullptr;

	wvpResource->Map(0,nullptr,reinterpret_cast<void**>(&wvpData));

#pragma endregion

#pragma region VertexBufferViewを生成する

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();

	vertexBufferView.SizeInBytes =
		UINT(sizeof(Sprite::VertexData) * modelData.vertices.size());

	vertexBufferView.StrideInBytes = sizeof(Sprite::VertexData);


#pragma endregion

#pragma region Textureの読み込み

	// UVチェッカーの読み込み
	// (内部でリソース作成や転送まで全自動で行われます)
	TextureManager::GetInstance()->LoadTexture("resource/uvChecker.png");

	// モデル用テクスチャの読み込み
	TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);

#pragma endregion

#pragma region 頂点データの更新

	// const uint32_t kSubdivision = 16;
	// const float kLonEvery = 2.0f * PI / float(kSubdivision);
	// const float kLatEvery = PI / float(kSubdivision);
	// const float epsilon = 1e-5f;

	Sprite::VertexData* vertexData = nullptr;
	vertexResource->Map(0,nullptr,reinterpret_cast<void**>(&vertexData));

	std::memcpy(vertexData,modelData.vertices.data(),
		sizeof(Sprite::VertexData) * modelData.vertices.size());

	// vertexResource->Unmap(0, nullptr);

	//// 頂点生成（(kSubdivision + 1)^2 個）
	// for (uint32_t latIndex = 0; latIndex <= kSubdivision; ++latIndex) {
	//   float lat = -PI / 2.0f + kLatEvery * latIndex;
	//   float v = 1.0f - float(latIndex) / float(kSubdivision);

	//  for (uint32_t lonIndex = 0; lonIndex <= kSubdivision; ++lonIndex) {
	//    float lon = lonIndex * kLonEvery;
	//    float u = float(lonIndex) / float(kSubdivision);
	//    if (lonIndex == kSubdivision) {
	//      u = 1.0f - epsilon; // wrap防止
	//    }

	//    Vector3 pos = {cosf(lat) * cosf(lon), sinf(lat), cosf(lat) * sinf(lon)};

	//    uint32_t vertexIndex = latIndex * (kSubdivision + 1) + lonIndex;

	//    vertexData[vertexIndex].position = {pos.x, pos.y, pos.z, 1.0f};
	//    vertexData[vertexIndex].normal = pos;
	//    vertexData[vertexIndex].texcoord = {u, v};
	//  }
	//}

	// uint32_t *indexData = nullptr;
	// indexResource->Map(0, nullptr, reinterpret_cast<void **>(&indexData));

	// uint32_t index = 0;
	// for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
	//   for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
	//     uint32_t i0 = latIndex * (kSubdivision + 1) + lonIndex;
	//     uint32_t i1 = i0 + 1;
	//     uint32_t i2 = i0 + (kSubdivision + 1);
	//     uint32_t i3 = i2 + 1;

	//    // 三角形 1
	//    indexData[index++] = i0;
	//    indexData[index++] = i2;
	//    indexData[index++] = i1;

	//    // 三角形 2
	//    indexData[index++] = i1;
	//    indexData[index++] = i2;
	//    indexData[index++] = i3;
	//  }
	//}

	uint32_t* indexData = nullptr;
	indexResource->Map(0,nullptr,reinterpret_cast<void**>(&indexData));
	std::memcpy(indexData,modelData.vertices.data(),
		sizeof(uint32_t) * modelData.vertices.size());

#pragma endregion

#pragma region XAudio2の初期化
	HRESULT result = XAudio2Create(&xAudio2,0,XAUDIO2_DEFAULT_PROCESSOR);
	result = xAudio2->CreateMasteringVoice(&masterVoice);
	assert(SUCCEEDED(result));
#pragma endregion

#pragma region 変数宣言

	Transform transform{
{1.0f, 1.0f, 1.0f},
{0.0f, 0.0f, 0.0f},
{0.0f, 0.0f, 0.0f},

	};

	Transform cameraTransform{
		{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -10.0f}};

	Transform uvTransformSprite{
		{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

	Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(
		0.45f,float(WinAPI::kCliantWidth) / float(WinAPI::kCliantHeight),0.1f,
		100.0f);

#pragma region ImGuiの初期化

	// 三角形の初期値
	Vector4 triangleColor = {1.0f, 1.0f, 1.0f, 1.0f};

	Sprite::Material* material = nullptr;

	bool useMonsterBall = false;

#pragma endregion

#pragma endregion
	while(true){

		if(winApi->ProcessMessage()){
			/// trueを返したときはゲームループを抜ける
			break;
		} else{
			// ゲームの処理

			// if (!hasPlayed) {
			//   SoundPlayWave(xAudio2.Get(), soundData);
			//   hasPlayed = true;
			// }

			sprite->Update();
			spriteBall->Update();

			//#ifdef _DEBUG
			//			ImGui_ImplDX12_NewFrame();
			//			ImGui_ImplWin32_NewFrame();
			//			ImGui::NewFrame();
			//
			//			ImGui::Begin("Settings");
			//
			//			// === Triangle Color ===
			//			if(ImGui::CollapsingHeader("model Color")){
			//				ImGui::ColorEdit4("Color",reinterpret_cast<float*>(&triangleColor));
			//			}
			//
			//			if(ImGui::CollapsingHeader("CameraTransform")){
			//				ImGui::DragFloat3("CameraTranslate",&cameraTransform.translate.x,
			//					0.1f);
			//				ImGui::SliderAngle("CameraRotateX",&cameraTransform.rotate.x);
			//				ImGui::SliderAngle("CameraRotateY",&cameraTransform.rotate.y);
			//				ImGui::SliderAngle("CameraRotateZ",&cameraTransform.rotate.z);
			//			}
			//
			//			// === Transform (SRT Controller) ===
			//			if(ImGui::CollapsingHeader("modelTransform")){
			//				ImGui::SliderAngle("RotateX",&transform.rotate.x);
			//				ImGui::SliderAngle("RotateY",&transform.rotate.y);
			//				ImGui::SliderAngle("RotateZ",&transform.rotate.z);
			//				// ImGui::Checkbox("useMonsterBall", &useMonsterBall);
			//				// materialData->enableLighting = useMonsterBall;
			//			}
			//
			//			if(ImGui::CollapsingHeader("Light Settings")){
			//				ImGui::Text("Directional Light");
			//
			//				ImGui::ColorEdit4(
			//					"Color",reinterpret_cast<float*>(&directionalLightData->color));
			//
			//				ImGui::SliderFloat3(
			//					"Direction",
			//					reinterpret_cast<float*>(&directionalLightData->direction),-1.0f,
			//					1.0f);
			//
			//				// Normalize direction
			//				Vector3& v = directionalLightData->direction;
			//				XMVECTOR dir = XMVectorSet(v.x,v.y,v.z,0.0f);
			//				dir = XMVector3Normalize(dir);
			//				XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&v),dir);
			//
			//				ImGui::SliderFloat("Intensity",&directionalLightData->intensity,0.0f,
			//					10.0f);
			//			}
			//
			//			// === Sprite Transform ===
			//			//if(ImGui::CollapsingHeader("Sprite Transform")){
			//			//	ImGui::DragFloat3("Scale##Sprite",&transformSprite.scale.x,0.1f,0.1f,
			//			//		10.0f);
			//			//	ImGui::DragFloat3("Rotate (rad)##Sprite",&transformSprite.rotate.x,
			//			//		0.01f,-3.14f,3.14f);
			//			//	ImGui::DragFloat3("Translate##Sprite",&transformSprite.translate.x,
			//			//		1.0f,-700.0f,700.0f);
			//			//}
			//
			//			// === UV Transform Sprite ===
			//			if(ImGui::CollapsingHeader("UV Transform Sprite")){
			//				ImGui::DragFloat2("UVTranslate",&uvTransformSprite.translate.x,0.01f,
			//					-10.0f,10.0f);
			//				ImGui::DragFloat2("UVScale",&uvTransformSprite.scale.x,0.01f,-10.0f,
			//					10.0f);
			//				ImGui::SliderAngle("UVRotate",&uvTransformSprite.rotate.z);
			//			}
			//
			//			ImGui::End();
			//			ImGui::Render();
			//
			//#endif

						///================================
						/// 更新処理
						/// ==============================

						//Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransformSprite.scale);

						//uvTransformMatrix = Multiply(
						//	uvTransformMatrix,MakeRotateZMatrix(uvTransformSprite.rotate.z));
						//uvTransformMatrix = Multiply(
						//	uvTransformMatrix,MakeTranslateMatrix(uvTransformSprite.translate));
						//	materialDataSprite->uvTransform = uvTransformMatrix;

#pragma region 三角形の回転

			//Matrix4x4 worldMatrix = MakeAffineMatrix(
			//	transform.scale,transform.rotate,transform.translate);
			//Matrix4x4 cameraMatrix =
			//	MakeAffineMatrix(cameraTransform.scale,cameraTransform.rotate,
			//		cameraTransform.translate);
			//Matrix4x4 viewMatrix = Inverse(cameraMatrix);
			//Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(
			//	0.45f,float(WinAPI::kCliantWidth) / float(WinAPI::kCliantHeight),
			//	0.1f,100.0f);
			//Matrix4x4 worldViewProjectionMatrix =
			//	Multiply(sprite->worldMatrix,Multiply(sprite->viewMatrix,projectionMatrix));

			////     transform.rotate.y += 0.01f;
			//*wvpData = {worldViewProjectionMatrix, sprite->worldMatrix};

#pragma endregion
			input->Update();

			if(input->TriggerKey(DIK_SPACE)){
				OutputDebugStringA("Trigger space\n");
			}

			///================================
			/// 描画処理
			///================================

			dxCommon->PreDraw();

			spriteCommon->Draw();

			// 三角形の色を変える
			//materialResource->Map(0,nullptr,reinterpret_cast<void**>(&material));
			//material->color = triangleColor;
			//materialResource->Unmap(0,nullptr);

			// 球の描画：Material, WVP, Light を正しくセット
			//dxCommon->commandList.Get()->IASetVertexBuffers(0,1,&vertexBufferView);
			//dxCommon->commandList.Get()->IASetIndexBuffer(&indexBufferView);
			//dxCommon->commandList.Get()->IASetPrimitiveTopology(
			//	D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			//dxCommon->commandList.Get()->SetGraphicsRootConstantBufferView(
			//	0,materialResource->GetGPUVirtualAddress());
			//dxCommon->commandList.Get()->SetGraphicsRootConstantBufferView(
			//	1,wvpResource->GetGPUVirtualAddress());
			//dxCommon->commandList.Get()->SetGraphicsRootConstantBufferView(
			//	3,directionalLightResource->GetGPUVirtualAddress());
			//dxCommon->commandList.Get()->SetGraphicsRootDescriptorTable(
			//	2,useMonsterBall?textureSrvHandleGPU2:textureSrvHandleGPU);
			////     uint32_t indexCount = kSubdivision * kSubdivision * 6;

			//dxCommon->commandList.Get()->DrawInstanced(
			//	UINT(modelData.vertices.size()),1,0,0);

			sprite->Draw();
			//	spriteBall->Draw();

				//	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(),
		//				dxCommon->commandList.Get());
			dxCommon->PostDraw();

#pragma endregion
		}
	}

	//#pragma region ImGuiの終了処理
	//	ImGui_ImplDX12_Shutdown();
	//	ImGui_ImplWin32_Shutdown();
	//	ImGui::DestroyContext();
	//#pragma endregion

	Log("unkillable demon king\n");

	TextureManager::GetInstance()->Finalize();

	delete spriteBall;
	spriteBall = nullptr;
	delete sprite;
	sprite = nullptr;
	delete spriteCommon;
	spriteCommon = nullptr;

	delete dxCommon;
	dxCommon = nullptr;

	delete input;
	input = nullptr;

	xAudio2.Reset();
	SoundUnload(&soundData);

	//  CloseHandle(fenceEvent);

	winApi->Finalize();
	delete winApi;
	winApi = nullptr;

	return 0;
}