cbuffer RayTraceConstants : register(b0)
{
    float4x4 ViewProjection;
    float4x4 InvViewProjection;
    float4 CameraPosWS;
    float2 yes[14];
};

inline RayDesc GenerateCameraRay(uint2 index, in float3 cameraPosition, in float4x4 projectionToWorld)
{
    float2 xy = index + 0.5f; // center in the middle of the pixel.
    float2 res = float2(1280.0, 800.0);
    float2 screenPos = xy / res * 2.0 - 1.0;

    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a world positon.
    float4 world = mul(float4(screenPos, 0, 1), projectionToWorld);
    world.xyz /= world.w;

    RayDesc ray;
    ray.Origin = cameraPosition;
    ray.Direction = normalize(world.xyz - ray.Origin);

    return ray;
}
float4 CalculateDiffuseLighting(float3 hitPosition, float3 normal)
{
    float3 pixelToLight = normalize(float3(10, 10, 10) - hitPosition);

    // Diffuse contribution.
    float fNDotL = max(0.0f, dot(pixelToLight, normal));

    return float4(0.7, 0.7, 0.7, 1) * float4(1, 1, 0.7, 1) * fNDotL;
}
RWTexture2D<float4> RenderTarget : register(u0);
RaytracingAccelerationStructure Scene : register(t0, space0);

[numthreads(1, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    RayDesc ray;
    ray = GenerateCameraRay(DTid.xy, CameraPosWS.xyz, InvViewProjection);
    // need to replace with viewport and width in constant buffer
    ray.TMin = 0.01;
    ray.TMax = 10000;
    
    RayQuery < RAY_FLAG_FORCE_OPAQUE > rayQuery;
    rayQuery.TraceRayInline(Scene, RAY_FLAG_NONE, 0xFF, ray);
    rayQuery.Proceed();
   
    if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        float3 rayhit = ray.Origin + (ray.Direction * rayQuery.CommittedRayT());
        float3 normal = RenderTarget[DTid.xy].xyz;
        if (rayhit.x < 0)
        {

        }
        else
        {

        }

        RenderTarget[DTid.xy] *= CalculateDiffuseLighting(rayhit, normal);


    }
    else
    {
        RenderTarget[DTid.xy] *= float4(0, 0, 0, 1.0);
    }

    GroupMemoryBarrierWithGroupSync();
}