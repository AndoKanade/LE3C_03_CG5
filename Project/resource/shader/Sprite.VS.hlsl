#include "Sprite.hlsli"

struct TransformationMatrix
{
    float32_t4x4 WVP;
    float32_t4x4 World;
};

// SpriteCommonの設定に合わせてレジスタを指定
// b0 = Material(Pixel用), b1 = Transform(Vertex用) のはずなので b1
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b1);

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    // 座標変換 (ここがメイン)
    output.position = mul(input.position, gTransformationMatrix.WVP);
    
    // テクスチャ座標はそのまま渡す
    output.texcoord = input.texcoord;
    
    return output;
}