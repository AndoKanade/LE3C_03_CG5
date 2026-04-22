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
	
	// 1. 元の色をサンプリング
    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    output.color = textureColor;

	// 2. UV座標を変換して画面中心を (0,0) にする
	// input.texcoord(0.0~1.0) -> ( -1.0~1.0 )
    float32_t2 centeredUV = input.texcoord;
    centeredUV -= 0.5f; // 中心を0にする
    centeredUV *= 2.0f; // 範囲を-1.0~1.0にする

	// 3. 中心からの距離を計算し、周辺に行くほど数値を大きくする
    float32_t distance = length(centeredUV);

	// 4. ビネット強度の計算
	// 距離が遠くなるほど小さい値をかける (saturateで0.0~1.0に収める)
	// 0.8を引くことで、中心付近は暗くならないように調整
    float32_t vignette = saturate(1.0f - distance * 0.5f);
	
	// 指数で減衰の仕方を滑らかにする
    vignette = pow(vignette, 0.8f);

	// 5. 元の色にビネット係数を掛ける
    output.color.rgb *= vignette;

    return output;
}