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

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}
