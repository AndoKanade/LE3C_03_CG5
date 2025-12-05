#pragma once
#include<Windows.h>
#include"DXCommon.h"

class SpriteCommon{

public:
	void Initialize(DXCommon* dxCommon);

	void Draw();

	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};

	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	D3D12_BLEND_DESC blendDesc{};
	D3D12_RASTERIZER_DESC resterizerDesc{};
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob;
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;

	DXCommon* GetDxCommon() const{ return dxCommon_; }

private:

	void CreateRootSignature();
	void CreatePipelineState();

	DXCommon* dxCommon_ = nullptr;

};

