struct PS_INPUT
{
    float4 Position : SV_POSITION;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    return float4(1.0f, 0.5f, 0.2f, 1.0f);
}