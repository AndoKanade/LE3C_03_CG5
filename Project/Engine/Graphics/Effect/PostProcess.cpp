#include "PostProcess.h"
#include <d3dx12.h>
#include <cassert>

// --- 初期化処理 ---
void PostProcess::Initialize(DXCommon* dxCommon){
	// 1. RootSignatureの生成（全シェーダー共通）
	CreateRootSignature(dxCommon->GetDevice());

	// 2. 種類ごとにPipelineStateを生成
	// 各種シェーダーファイルを読み込み、pipelineStates_マップに登録
	CreatePipelineState(dxCommon->GetDevice(),dxCommon,Type::PostProcess,L"Engine/Graphics/Shaders/PostProcess/PostProcess.PS.hlsl");
	CreatePipelineState(dxCommon->GetDevice(),dxCommon,Type::BoxFilter,L"Engine/Graphics/Shaders/PostProcess/BoxFilter.PS.hlsl");
	CreatePipelineState(dxCommon->GetDevice(),dxCommon,Type::Grayscale,L"Engine/Graphics/Shaders/PostProcess/Grayscale.PS.hlsl");
	CreatePipelineState(dxCommon->GetDevice(),dxCommon,Type::Vignette,L"Engine/Graphics/Shaders/PostProcess/Vignette.PS.hlsl");
	CreatePipelineState(dxCommon->GetDevice(),dxCommon,Type::GaussianBlur,L"Engine/Graphics/Shaders/PostProcess/GaussianBlur.PS.hlsl");
	CreatePipelineState(dxCommon->GetDevice(),dxCommon,Type::Outline,L"Engine/Graphics/Shaders/PostProcess/LuminanceBasedOutline.PS.hlsl");

	// 3. 定数バッファの生成とマッピング
	constBuff_ = dxCommon->CreateBufferResource(sizeof(PostProcessData));
	constBuff_->Map(0,nullptr,reinterpret_cast<void**>(&constMap_));

	// 4. 初期パラメータの設定
	constMap_->kernelSize = 1;
	constMap_->vignetteIntensity = 0.5f;
	constMap_->vignetteScale = 0.8f;
}

// --- 描画処理 ---
void PostProcess::Draw(ID3D12GraphicsCommandList* commandList,
	D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
	D3D12_GPU_DESCRIPTOR_HANDLE depthTextureHandle, // ★追加
	Type type){
	commandList->SetPipelineState(pipelineStates_[type].Get());
	commandList->SetGraphicsRootSignature(rootSignature_.Get());

	// リソースのバインド
	commandList->SetGraphicsRootDescriptorTable(0,textureHandle);
	commandList->SetGraphicsRootConstantBufferView(1,constBuff_->GetGPUVirtualAddress());
	commandList->SetGraphicsRootDescriptorTable(2,depthTextureHandle); 
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(3,1,0,0);
}

// --- 内部関数: ルートシグネチャ生成 ---
void PostProcess::CreateRootSignature(ID3D12Device* device){
	HRESULT hr;
	D3D12_ROOT_PARAMETER rootParameters[3] = {};

	// [0]: DescriptorTable (SRV用: register(t0)) - 元画像用
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].BaseShaderRegister = 0; // t0
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].DescriptorTable.pDescriptorRanges = &descriptorRange[0];
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// [1]: CBV (ConstantBuffer用: register(b1)) - 定数バッファ用
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].Descriptor.ShaderRegister = 1; // b1
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// [2]: DescriptorTable (SRV用: register(t1)) - 深度テクスチャ用 
	D3D12_DESCRIPTOR_RANGE depthDescriptorRange[1] = {};
	depthDescriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	depthDescriptorRange[0].NumDescriptors = 1;
	depthDescriptorRange[0].BaseShaderRegister = 1; // t1
	depthDescriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].DescriptorTable.pDescriptorRanges = &depthDescriptorRange[0];
	rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// 静的サンプラーの設定 (s0)
	D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ShaderRegister = 0; // s0
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	staticSamplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT; // 補間しない
	staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[1].ShaderRegister = 1; // s1
	staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// ルートシグネチャの構成
	D3D12_ROOT_SIGNATURE_DESC description = {};
	description.pParameters = rootParameters;
	description.NumParameters = _countof(rootParameters);
	description.pStaticSamplers = staticSamplers;
	description.NumStaticSamplers = _countof(staticSamplers);
	description.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// シリアライズと生成
	ComPtr<ID3DBlob> signatureBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&description,D3D_ROOT_SIGNATURE_VERSION_1,&signatureBlob,&errorBlob);
	if(FAILED(hr)){ assert(false); }

	hr = device->CreateRootSignature(0,signatureBlob->GetBufferPointer(),signatureBlob->GetBufferSize(),IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));
}

// --- 内部関数: パイプラインステート生成 ---
void PostProcess::CreatePipelineState(ID3D12Device* device,DXCommon* dxCommon,Type type,const std::wstring& filename){
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

	// シェーダーのコンパイル
	auto vsBlob = dxCommon->CompileShader(L"Engine/Graphics/Shaders/PostProcess/PostProcess.VS.hlsl",L"vs_6_0");
	auto psBlob = dxCommon->CompileShader(filename,L"ps_6_0");
	psoDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
	psoDesc.PS = {psBlob->GetBufferPointer(), psBlob->GetBufferSize()};

	// 頂点レイアウトと深度設定（ポストプロセスのため不使用）
	psoDesc.InputLayout.pInputElementDescs = nullptr;
	psoDesc.InputLayout.NumElements = 0;
	psoDesc.DepthStencilState.DepthEnable = false;

	// ラスタライザ・ブレンド・トポロジー設定
	psoDesc.pRootSignature = rootSignature_.Get();
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// レンダーターゲット設定
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDesc.SampleDesc.Count = 1;

	// PSOの生成とマップへの保存
	ComPtr<ID3D12PipelineState> pipelineState;
	HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc,IID_PPV_ARGS(&pipelineState));
	assert(SUCCEEDED(hr));

	pipelineStates_[type] = pipelineState;
}