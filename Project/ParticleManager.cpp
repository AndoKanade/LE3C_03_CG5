#include "ParticleManager.h"
#include "TextureManager.h" // TextureManagerが必要になります
#include"Camera.h"
#include"Logger.h"
#include <cassert>

// シングルトン取得などは省略...
using namespace Microsoft::WRL;
// ■ static変数の定義
ParticleManager* ParticleManager::instance = nullptr;

// ■ GetInstanceの書き換え (TextureManagerと同じ形に)
ParticleManager* ParticleManager::GetInstance(){
	if(instance == nullptr){
		instance = new ParticleManager();
	}
	return instance;
}

// ■ Finalizeの実装
void ParticleManager::Finalize(){
	// インスタンスを削除 (これでデストラクタが走り、ComPtrなどが解放される)
	delete instance;
	instance = nullptr;
}


// ■ 初期化関数の書き換え
void ParticleManager::Initialize(DXCommon* dxCommon,SrvManager* srvManager){
	// 1. ポインタの受取
	assert(dxCommon);
	assert(srvManager);
	dxCommon_ = dxCommon;
	srvManager_ = srvManager;

	// 2. ランダムエンジンの初期化
	std::random_device seedGenerator;
	randomEngine_.seed(seedGenerator());

	CreateGraphicsPipeline();

	// モデル生成（四角形の頂点バッファを作る）
	CreateModel();
}

// ■ 追加: パーティクルグループの生成関数
void ParticleManager::CreateParticleGroup(const std::string& name,const std::string& textureFilePath){

	// ● 登録済みの名前かチェックしてassert
	// (すでに同じ名前のグループがあったら止める)
	assert(!particleGroups_.contains(name));

	// ● 新たな空のパーティクルグループを作成し、コンテナに登録
	// (マップに新しい要素を追加し、参照を取得)
	ParticleGroup& group = particleGroups_[name];

	// ● 新たなパーティクルグループの...
	// ○ マテリアルデータにテクスチャファイルパスを設定
	group.textureFilePath = textureFilePath;

	// ○ テクスチャを読み込む
	TextureManager::GetInstance()->LoadTexture(textureFilePath);

	// ○ マテリアルデータにテクスチャのSRVインデックスを記録
	group.textureSrvIndex = TextureManager::GetInstance()->GetSrvIndex(textureFilePath);

	// ○ インスタンシング用リソースの生成
	// (GPUに送るデータを格納するバッファを作る)
	group.instancingResource = dxCommon_->CreateBufferResource(sizeof(ParticleForGPU) * group.kNumMaxInstance);

	// (※データを書き込むためのポインタもここで取得しておきます)
	group.instancingResource->Map(0,nullptr,reinterpret_cast<void**>(&group.instancingData));

	// ○ インスタンシング用にSRVを確保してSRVインデックスを記録
	group.instancingSrvIndex = srvManager_->Allocate();

	// ○ SRV生成 (StructuredBuffer用設定)
	D3D12_SHADER_RESOURCE_VIEW_DESC instancingSrvDesc{};
	instancingSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	instancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	instancingSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	instancingSrvDesc.Buffer.FirstElement = 0;
	instancingSrvDesc.Buffer.NumElements = group.kNumMaxInstance;
	instancingSrvDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);
	instancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	srvManager_->CreateSRVforStructuredBuffer(
		group.instancingSrvIndex,
		group.instancingResource.Get(),
		instancingSrvDesc.Buffer.NumElements,
		instancingSrvDesc.Buffer.StructureByteStride
	);
}

void ParticleManager::Update(Camera* camera){
	assert(camera); // カメラがないと計算できないのでチェック

	const float kDeltaTime = 1.0f / 60.0f; // 60FPS想定

	// ---------------------------------------------------------
	// 1. 行列の事前準備 (ループの外でやる処理)
	// ---------------------------------------------------------

	// ビュー行列とプロジェクション行列をカメラから取得
	Matrix4x4 viewMatrix = camera->GetViewMatrix();
	Matrix4x4 projectionMatrix = camera->GetProjectionMatrix();

	// 2つを合成してVP行列を作っておく
	Matrix4x4 viewProjectionMatrix = Multiply(viewMatrix,projectionMatrix);

	// ビルボード行列の計算
	// (カメラのワールド行列の回転成分だけを取り出すことで、カメラと同じ向き＝カメラの方を向く行列になる)
	Matrix4x4 billboardMatrix = camera->GetWorldMatrix();
	billboardMatrix.m[3][0] = 0.0f; // 平行移動成分を削除
	billboardMatrix.m[3][1] = 0.0f;
	billboardMatrix.m[3][2] = 0.0f;


	// ---------------------------------------------------------
	// 2. 全てのパーティクルグループについて処理する (二重ループの外側)
	// ---------------------------------------------------------
	for(auto& [name,group] : particleGroups_){

		// インスタンシングデータの書き込み位置リセット
		uint32_t numInstance = 0;

		// ---------------------------------------------------------
		// 3. グループ内の全てのパーティクルについて処理する (二重ループの内側)
		// ---------------------------------------------------------
		for(auto it = group.particles.begin(); it != group.particles.end(); ){

			// ■ 寿命に達していたらグループから外す
			if(it->lifeTime <= it->currentTime){
				it = group.particles.erase(it);
				continue; // 削除したので次のループへ
			}

			// --- 生きているパーティクルの処理 ---

			// ■ 場の影響を計算（加速）
			// ※AccelerationFieldなどがメンバにあればここで計算
			// 例: it->velocity += accelerationField_.acceleration * kDeltaTime;

			// ■ 移動処理（速度を座標に加算）
			it->transform.translate += it->velocity * kDeltaTime;

			// ■ 経過時間を加算
			it->currentTime += kDeltaTime;

			// 透明度の計算 (寿命に応じてフェードアウト)
			float alpha = 1.0f - (it->currentTime / it->lifeTime);

			// ■ ワールド行列を計算
			// ビルボード行列を使うため、回転行列の代わりに billboardMatrix を掛け合わせる
			// World = Scale * Billboard * Translate
			Matrix4x4 scaleMatrix = MakeScaleMatrix(it->transform.scale);
			Matrix4x4 translateMatrix = MakeTranslateMatrix(it->transform.translate);

			// 行列の掛け合わせ (Scale * Billboard * Translate)
			Matrix4x4 worldMatrix = Multiply(scaleMatrix,Multiply(billboardMatrix,translateMatrix));

			// ■ ワールドビュープロジェクション行列を合成
			Matrix4x4 wvpMatrix = Multiply(worldMatrix,viewProjectionMatrix);


			// ■ インスタンシング用データ1個分の書き込み
			// ※バッファオーバーラン対策をしておく
			if(numInstance < group.kNumMaxInstance){
				group.instancingData[numInstance].WVP = wvpMatrix;
				group.instancingData[numInstance].World = worldMatrix;
				group.instancingData[numInstance].color = it->color;
				group.instancingData[numInstance].color.w = alpha; // 透明度反映

				++numInstance; // 書き込んだ数をカウント
			}

			++it; // 次のパーティクルへ
		}
		// ループ終了後、このグループの描画準備は完了
		group.numInstance = numInstance;
	}
}

void ParticleManager::Draw(const Matrix4x4& viewProjectionMatrix){
	assert(dxCommon_);
	// コマンドリストの取得
	auto commandList = dxCommon_->GetCommandList();

	// -----------------------------------------------------------
	// 共通設定
	// -----------------------------------------------------------

	// ● ルートシグネチャを設定
	commandList->SetGraphicsRootSignature(rootSignature_.Get());

	// ● PSOを設定
	commandList->SetPipelineState(graphicsPipelineState_.Get());

	// ● プリミティブトポロジーを設定
	// 板ポリゴン（4頂点）を描画するため、TRIANGLESTRIPを使用します
	// (※インデックスバッファを作っている場合はTRIANGLELIST + DrawIndexedInstancedになります)
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	// ● VBVを設定
	commandList->IASetVertexBuffers(0,1,&vertexBufferView_);

	// -----------------------------------------------------------
	// ● 全てのパーティクルグループについて処理する
	// -----------------------------------------------------------
	for(auto& [name,group] : particleGroups_){

		// 描画するインスタンスがない（0個）ならスキップ
		if(group.numInstance == 0){
			continue;
		}

		// ○ テクスチャのSRVのDescriptorTableを設定
		// SrvManagerからGPUハンドルを取得
		D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = srvManager_->GetGPUDescriptorHandle(group.textureSrvIndex);

		// 【重要】ここの "0" はルートシグネチャのパラメータ番号です。ご自身の環境に合わせて変更してください。
		commandList->SetGraphicsRootDescriptorTable(0,textureSrvHandleGPU);


		// ○ インスタンシングデータのSRVのDescriptorTableを設定
		D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU = srvManager_->GetGPUDescriptorHandle(group.instancingSrvIndex);

		// 【重要】ここの "1" もルートシグネチャのパラメータ番号です。StructuredBufferのレジスタ番号に合わせてください。
		commandList->SetGraphicsRootDescriptorTable(1,instancingSrvHandleGPU);


		// ○ DrawCall (インスタンシング描画)
		// 第1引数: 1個あたりの頂点数 (矩形なので4)
		// 第2引数: インスタンス数 (Updateで計算した group.numInstance)
		commandList->DrawInstanced(4,group.numInstance,0,0);
	}
}
// ---------------------------------------------------------
// ■ 1. MakeNewParticleの実装
// (今までmain.cppにあった処理をここに移植し、メンバ変数 randomEngine_ を使うように変更)
// ---------------------------------------------------------
Particle ParticleManager::MakeNewParticle(const Vector3& translate){
	Particle particle;

	// メンバ変数の randomEngine_ を使う
	std::uniform_real_distribution<float> distributionPos(-1.0f,1.0f);
	std::uniform_real_distribution<float> distributionVel(-1.0f,1.0f);
	std::uniform_real_distribution<float> distColor(0.0f,1.0f);
	std::uniform_real_distribution<float> distTime(1.0f,3.0f);

	// 位置 (引数の translate を基準にランダムにずらす)
	Vector3 randomTranslate = {distributionPos(randomEngine_), distributionPos(randomEngine_), distributionPos(randomEngine_)};
	particle.transform.translate = translate + randomTranslate;

	// 速度
	particle.velocity = {distributionVel(randomEngine_), distributionVel(randomEngine_), distributionVel(randomEngine_)};

	// 色
	particle.color = {distColor(randomEngine_), distColor(randomEngine_), distColor(randomEngine_), 1.0f};

	// 寿命と時間
	particle.lifeTime = distTime(randomEngine_);
	particle.currentTime = 0.0f;

	// スケールや回転の初期化も忘れずに
	particle.transform.scale = {1.0f, 1.0f, 1.0f};
	particle.transform.rotate = {0.0f, 0.0f, 0.0f};

	return particle;
}

// ---------------------------------------------------------
// ■ 2. Emitの実装
// ---------------------------------------------------------
void ParticleManager::Emit(const std::string& name,const Vector3& position,uint32_t count){
	// 登録済みの名前かチェック
	assert(particleGroups_.contains(name));

	// グループの参照を取得
	ParticleGroup& group = particleGroups_[name];

	// 指定された数だけパーティクルを生成してリストに追加
	for(uint32_t i = 0; i < count; ++i){
		// 上で作った MakeNewParticle を呼ぶ
		Particle newParticle = MakeNewParticle(position);
		group.particles.push_back(newParticle);
	}
}


void ParticleManager::CreateGraphicsPipeline(){
	HRESULT hr = S_OK;

	// ==========================================================
	// 1. RootSignature の作成
	// ==========================================================
	D3D12_DESCRIPTOR_RANGE descriptorRangeTexture[1] = {};
	descriptorRangeTexture[0].BaseShaderRegister = 0; // t0
	descriptorRangeTexture[0].NumDescriptors = 1;
	descriptorRangeTexture[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRangeTexture[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE descriptorRangeInstancing[1] = {};
	descriptorRangeInstancing[0].BaseShaderRegister = 0; // t0 (VSで使う)
	descriptorRangeInstancing[0].NumDescriptors = 1;
	descriptorRangeInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRangeInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// RootParameter設定 (Draw関数の設定順序と合わせる)
	// [0]: テクスチャ (PixelShader)
	// [1]: インスタンシングデータ (VertexShader)
	D3D12_ROOT_PARAMETER rootParameters[2] = {};

	// [0] Texture
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRangeTexture;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeTexture);

	// [1] Instancing Data (StructuredBuffer)
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRangeInstancing;
	rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeInstancing);

	// サンプラー設定
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// ルートシグネチャ設定
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	// シリアライズ
	ComPtr<ID3DBlob> signatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature,D3D_ROOT_SIGNATURE_VERSION_1,&signatureBlob,&errorBlob);
	if(FAILED(hr)){
		Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	// 生成
	hr = dxCommon_->GetDevice()->CreateRootSignature(0,signatureBlob->GetBufferPointer(),signatureBlob->GetBufferSize(),IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));


	// ==========================================================
	// 2. GraphicsPipelineState (PSO) の作成
	// ==========================================================

	// シェーダーコンパイル (Obj3dCommonに合わせて dxCommon_->CompileShader を使用)
	// ※シェーダーファイル名は適宜変更してください
	ComPtr<IDxcBlob> vertexShaderBlob = dxCommon_->CompileShader(L"resource/shader/Particle.VS.hlsl",L"vs_6_0");
	assert(vertexShaderBlob != nullptr);
	ComPtr<IDxcBlob> pixelShaderBlob = dxCommon_->CompileShader(L"resource/shader/Particle.PS.hlsl",L"ps_6_0");
	assert(pixelShaderBlob != nullptr);

	// InputLayout (Obj3dと同じでOK)
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	// BlendState (【重要】パーティクル用に変更：加算合成の設定)
	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	// 加算合成 (光る表現)
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

	// RasterizerState (カリングなし＝両面描画)
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// DepthStencilState (【重要】Z書き込みをOFFにする)
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // 書き込まない
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// PSO生成設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get();
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()};
	graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()};
	graphicsPipelineStateDesc.BlendState = blendDesc;
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// 生成
	hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,IID_PPV_ARGS(&graphicsPipelineState_));
	assert(SUCCEEDED(hr));
}

// ■ 四角形の頂点データを作る関数
void ParticleManager::CreateModel(){
	// 頂点データ (4頂点で四角形を作る)
	VertexData vertices[4] = {
		// 左下
		{{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
		// 左上
		{{-1.0f,  1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
		// 右下
		{{ 1.0f, -1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
		// 右上
		{{ 1.0f,  1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
	};

	// 頂点バッファの作成
	vertexBuffer_ = dxCommon_->CreateBufferResource(sizeof(vertices));

	// データを書き込む
	VertexData* vertexData = nullptr;
	vertexBuffer_->Map(0,nullptr,reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData,vertices,sizeof(vertices));
	vertexBuffer_->Unmap(0,nullptr);

	// ビューの作成
	vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(vertices);
	vertexBufferView_.StrideInBytes = sizeof(VertexData);
}