cbuffer RayTraceConstants : register(b0)
{
    float4x4 InvViewProjection;
    float4 CameraPosWS;
    float2 yes[22];
};

struct PSInput
{
    float4 position : SV_POSITION;
};

PSInput VSMain(float4 position : POSITION)
{
    PSInput result;

    result.position = mul(float4(position.xyz, 1.0f), InvViewProjection);

    return result;
}