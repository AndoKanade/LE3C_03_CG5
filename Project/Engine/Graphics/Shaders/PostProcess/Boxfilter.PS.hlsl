#include "PostProcess.hlsli"

// C++から送られてくるデータ
struct PostProcessData
{
    int32_t kernelSize; // kの値 (1なら3x3, 2なら5x5)
};

ConstantBuffer<PostProcessData> gData : register(b0); // b0を使用
Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    float32_t2 uvStepSize = float32_t2(rcp(float32_t(width)), rcp(float32_t(height)));

    PixelShaderOutput output;
    float32_t3 sum = float32_t3(0.0f, 0.0f, 0.0f);
    
    // カーネルサイズの取得 (ImGuiでいじる変数)
    int32_t k = gData.kernelSize;
    
    // 合計ピクセル数を計算 (k=1なら9, k=2なら25)
    float32_t numPixels = float32_t((2 * k + 1) * (2 * k + 1));

    // 動的なループ
    for (int32_t i = -k; i <= k; ++i)
    {
        for (int32_t j = -k; j <= k; ++j)
        {
            float32_t2 texcoord = input.texcoord + float32_t2(float32_t(i), float32_t(j)) * uvStepSize;
            sum += gTexture.Sample(gSampler, texcoord).rgb;
        }
    }

    // 平均化して出力
    output.color.rgb = sum / numPixels;
    output.color.a = 1.0f;

    return output;
}