#include "CopyImage.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    // テクスチャから色を抜いてくるだけ（コピー）
    output.color = gTexture.Sample(gSampler, input.texcoord);
    return output;
}