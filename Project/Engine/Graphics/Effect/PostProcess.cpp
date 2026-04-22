#include "PostProcess.h"

void PostProcess::Initialize(DXCommon* dxCommon){
	// RootSignatureを先に生成してからPipelineStateを作る
	CreateRootSignature(dxCommon->GetDevice());
	CreatePipelineState(dxCommon->GetDevice(),dxCommon);

	constBuff_ = dxCommon->CreateBufferResource(sizeof(PostProcessData));
	constBuff_->Map(0,nullptr,reinterpret_cast<void**>(&constMap_));
	constMap_->kernelSize = 1;
}

void PostProcess::CreateRootSignature(ID3D12Device* device){
	HRESULT hr;

	// ルートパラメータの設定 (3つのパラメータを使用)
	D3D12_ROOT_PARAMETER rootParameters[2] = {};

	// 0: DescriptorTable (SRV用: register(t0))
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].DescriptorTable.pDescriptorRanges = &descriptorRange[0];
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// 1: CBV (ConstantBuffer用: register(b0))
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].Descriptor.ShaderRegister = 0;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// 静的サンプラーの設定 (s0: Sampler)
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// ルートシグネチャの構成
	D3D12_ROOT_SIGNATURE_DESC description = {};
	description.pParameters = rootParameters;
	description.NumParameters = _countof(rootParameters);
	description.pStaticSamplers = staticSamplers;
	description.NumStaticSamplers = 1;
	description.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// シリアライズと生成
	ComPtr<ID3DBlob> signatureBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&description,D3D_ROOT_SIGNATURE_VERSION_1,&signatureBlob,&errorBlob);
	if(FAILED(hr)){
		assert(false);
	}

	hr = device->CreateRootSignature(0,signatureBlob->GetBufferPointer(),signatureBlob->GetBufferSize(),IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));
}

void PostProcess::CreatePipelineState(ID3D12Device* device,DXCommon* dxCommon){
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

	// シェーダーのコンパイルとセット
	auto vsBlob = dxCommon->CompileShader(L"Engine/Graphics/Shaders/PostProcess/PostProcess.VS.hlsl",L"vs_6_0");
	auto psBlob = dxCommon->CompileShader(L"Engine/Graphics/Shaders/PostProcess/Boxfilter.PS.hlsl",L"ps_6_0");
	psoDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
	psoDesc.PS = {psBlob->GetBufferPointer(), psBlob->GetBufferSize()};

	// 頂点データを使用しないための設定
	psoDesc.InputLayout.pInputElementDescs = nullptr;
	psoDesc.InputLayout.NumElements = 0;

	// 深度テストを使用しない設定
	psoDesc.DepthStencilState.DepthEnable = false;

	// パイプライン状態の基本設定
	psoDesc.pRootSignature = rootSignature_.Get();
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDesc.SampleDesc.Count = 1;

	// PSOの生成
	HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc,IID_PPV_ARGS(&pipelineState_));
	assert(SUCCEEDED(hr));
}

void PostProcess::Draw(ID3D12GraphicsCommandList* commandList,D3D12_GPU_DESCRIPTOR_HANDLE textureHandle){
	// 各種リソースのセット
	commandList->SetPipelineState(pipelineState_.Get());
	commandList->SetGraphicsRootSignature(rootSignature_.Get());

	// 0番にSRV（テクスチャ）をセット
	commandList->SetGraphicsRootDescriptorTable(0,textureHandle);

	// 1番にCBV（定数バッファ）をセット
	commandList->SetGraphicsRootConstantBufferView(1,constBuff_->GetGPUVirtualAddress());

	// トポロジーをセットして描画
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(3,1,0,0);
}