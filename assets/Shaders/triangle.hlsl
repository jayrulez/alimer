#include "assets\Shaders\Common.hlsl"

struct VS_INPUT
{
    VERTEX_ATTRIBUTE(float3, Position, 0);
    VERTEX_ATTRIBUTE(float4, Color, 1);
    VERTEX_ATTRIBUTE(float2, TexCoord0, 2);
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 Color: COLOR0;
    float2 TexCoord0 : TEXCOORD0;
};

cbuffer ProjectionMatrixBuffer : register(b0) 
{
    float4x4 mvp; 
};

Texture2D Texture0 : register(t0);
sampler Sampler0 : register(s0);

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;
    //output.Position = float4(input.Position.xyz, 1.0f);
    output.Position = mul(mvp, float4(input.Position.xyz, 1.0f));
    output.Color = input.Color;
    output.TexCoord0 = input.TexCoord0;
    return output;
}

float4 PSMain(PS_INPUT input) : SV_Target
{
    return Texture0.Sample(Sampler0, input.TexCoord0) * input.Color;
}
