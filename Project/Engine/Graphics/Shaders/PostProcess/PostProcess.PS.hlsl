#include "PostProcess.hlsli"

// --- 定数バッファ (register b1) ---
cbuffer PostProcessConfig : register(b1)
{
    int32_t gKernelSize; // 使用しないが定義が必要
    float gVignetteIntensity; // 使用しないが定義が必要
    float gVignetteScale; // 使用しないが定義が必要
    float gPadding; // パディング
};

// --- リソース (Texture & Sampler) ---
Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// --- 構造体 ---
struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

// --- メイン関数 ---
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // 1. テクスチャから元の色をそのままサンプリング
    // 何も加工せず出力することで、ポストプロセス無しの状態を作る
    output.color = gTexture.Sample(gSampler, input.texcoord);
    
    return output;
}