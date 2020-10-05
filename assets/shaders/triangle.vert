struct VS_INPUT
{
    [[vk::location(0)]] float3 Position : POSITION0;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    output.Position = float4(input.Position.xyz, 1.0f);
    return output;
}