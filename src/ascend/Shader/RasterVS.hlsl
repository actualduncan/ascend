cbuffer RayTraceConstants : register(b0)
{
    float4x4 ViewProjection;
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

PSInput VSMain(float4 position : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD)
{
    PSInput result;

    result.position = mul(float4(position.xyz, 1.0f), ViewProjection);
    result.normal = normal;
    result.uv = uv;
    return result;
}