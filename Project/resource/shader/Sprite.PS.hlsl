#include "Sprite.hlsli"

struct Material
{
    float32_t4 color;
    int32_t enableLighting;
    float32_t4x4 uvTransform;
    float32_t shininess;
};

// ★ここがポイント：SpriteCommonが送ってこない Light(b2) と Camera(b3) を消しました
ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // 1. UV変換
    float4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);

    // 2. テクスチャサンプリング
    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    // 3. 色の計算 (ライティングなし！ただの色×画像の色)
    output.color = gMaterial.color * textureColor;

    // 4. 透明度チェック (完全に透明なピクセルは描画しない)
    if (textureColor.a == 0.0f)
    {
        discard;
    }
    if (output.color.a == 0.0f)
    {
        discard;
    }

    return output;
}