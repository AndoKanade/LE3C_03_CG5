#include "PostProcess.hlsli"

// --- 定数バッファ (register b1) ---
cbuffer PostProcessConfig : register(b1)
{
    int32_t gKernelSize; // Grayscale不使用
    float gVignetteIntensity; // Grayscale不使用
    float gVignetteScale; // Grayscale不使用
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
    
    // 1. テクスチャのサンプリング
    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    
    // 2. 輝度の計算 (Rec.709 係数)
    // RGBそれぞれの寄与度を dot 関数で一気に計算
    float32_t3 luminanceWeights = float32_t3(0.2125f, 0.7154f, 0.0721f);
    float32_t luminance = dot(textureColor.rgb, luminanceWeights);
    
    // 3. 出力色の構成
    output.color.rgb = (float32_t3) luminance; // 輝度値をRGBすべてにコピー
    output.color.a = textureColor.a; // 元のアルファ値を維持
    
    return output;
}