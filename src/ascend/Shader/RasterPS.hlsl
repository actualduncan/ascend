
cbuffer RayTraceConstants : register(b0)
{
    float4x4 InvViewProjection;
    float4 CameraPosWS;
    float2 yes[22];
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 normal : NORMAL;
    float2 uv : TEXCOORD;
};
struct PSOutput
{
    float4 Color : SV_TARGET0;
    float4 Normal : SV_Target1;
};
Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSOutput PSMain(PSInput input)
{
    PSOutput output;
    output.Color = g_texture.Sample(g_sampler, input.uv) * float4(1, 1, 1, 1); //Set first output
    output.Normal = input.normal; //Set second output

    return output;
}
