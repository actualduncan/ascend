cbuffer ProjectionMatrixBuffer : register(b0)
{
    float4x4 ProjectionMatrix;
};

struct VSInput
{
    float2 position : POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = mul(ProjectionMatrix, float4(input.position.xy, 0.f, 1.f));
    output.color = input.color;
    output.uv = input.uv;
    return output;
}