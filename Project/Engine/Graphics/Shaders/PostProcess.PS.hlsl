#include "PostProcess.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // テクスチャから元の色をサンプリング
    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    
    // 輝度の計算 (Rec.709)
    float32_t luminance = dot(textureColor.rgb, float32_t3(0.2125f, 0.7154f, 0.0721f));
    
    // 輝度をRGBに適用し、アルファ値は元の色を維持
    output.color.rgb = float32_t3(luminance, luminance, luminance);
    output.color.a = textureColor.a;
    
    return output;
}