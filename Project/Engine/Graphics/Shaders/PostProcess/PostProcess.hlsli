struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD0;
};

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

cbuffer PostProcessConfig : register(b1)
{
    int32_t gKernelSize;
    float gVignetteIntensity;
    float gVignetteScale;
    float gPadding;
    float2 radialBlurCenter;
    float radialBlurWidth;
};