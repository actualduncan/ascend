
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

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

float4 PSMain(PSInput input) : SV_TARGET
{
    return g_texture.Sample(g_sampler, input.uv) * float4(1, 1, 1, 1);
}
