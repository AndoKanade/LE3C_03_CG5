#include"Particle.hlsli"

struct Material
{
    float32_t4 color;
    int32_t enableLighting;
    float32_t4x4 uvTransform;
    
};

struct DirectionalLight
{
    float32_t4 color;
    float32_t3 direction;
    float intensity;
    
};

ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b2);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
   
    float32_t4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    
    // ライティング有効の場合
    if (gMaterial.enableLighting != 0)
    {
        float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);

        // rgb に input.color.rgb を掛ける
        output.color.rgb = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity * input.color.rgb; // ← 末尾に追加

        // alpha に input.color.a を掛ける
        output.color.a = gMaterial.color.a * textureColor.a * input.color.a; // ← 末尾に追加
    }
    // ライティング無効の場合
    else
    {
        // 全体に input.color を掛ける
        output.color = gMaterial.color * textureColor * input.color; // ← 末尾に追加
    }
    
    // --- 以下は変更なし ---
    if (textureColor.a <= 0.5f)
    {
        discard;
    }
    
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