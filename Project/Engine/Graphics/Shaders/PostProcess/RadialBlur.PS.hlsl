#include "PostProcess.hlsli"

// --- 定数バッファ (register b1) ---
// 他のポストプロセス用シェーダーと構造を完全に一致させる
cbuffer PostProcessConfig : register(b1)
{
    int32_t gKernelSize; // ボックスフィルタ等で使用
    float gVignetteIntensity; // ビネットで使用
    float gVignetteScale; // ビネットで使用
    float gPadding; // 16バイトアライメント用パディング
    float2 radialBlurCenter; // 放射状ブラーの中心点
    float radialBlurWidth; // 放射状ブラーの拡散幅
};

// --- リソース ---
Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSamplerLinear : register(s0);

// --- メイン処理 ---
PixelShaderOutput main(VertexShaderOutput input)
{
    // サンプリング回数（固定値）
    const int32_t kNumSamples = 10;
    
    // 現在のUV座標から中心点への方向ベクトルを計算
    // 中心から離れるほどベクトルが長くなり、ブラーが強くかかる
    float32_t2 direction = input.texcoord - radialBlurCenter;
    
    float32_t3 colorSum = (float32_t3) 0.0f;

    // 指定したサンプル数分、方向ベクトルに沿って色を取り込む
    for (int32_t i = 0; i < kNumSamples; ++i)
    {
        // 徐々にサンプリング地点を中心から外側（または逆）へずらしていく
        float32_t2 offset = direction * radialBlurWidth * float32_t(i);
        colorSum += gTexture.Sample(gSamplerLinear, input.texcoord + offset).rgb;
    }

    // 平均化して出力
    PixelShaderOutput output;
    output.color.rgb = colorSum / float32_t(kNumSamples);
    output.color.a = 1.0f;
    
    return output;
}