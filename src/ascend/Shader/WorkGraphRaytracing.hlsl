cbuffer RayTraceConstants : register(b0)
{
    float4x4 ViewProjection;
    float4x4 InvViewProjection;
    float4 CameraPosWS;
    float4 lightPos;
    float2 yes[12];
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
    float3 pixelToLight = normalize(lightPos.xyz - hitPosition);

    // Diffuse contribution.
    float fNDotL = max(0.0f, abs(dot(pixelToLight, normal)));

    return float4(0.7, 0.7, 0.7, 1) * float4(1, 1, 0.7, 1) * fNDotL;
}

RWTexture2D<float4> RenderTarget : register(u0);
RWTexture2D<float4> Normals : register(u1);
RWTexture2D<float> Depth : register(u2);
RaytracingAccelerationStructure Scene : register(t0, space0);

[Shader("node")]
[NodeLaunch("broadcasting")]
[NodeIsProgramEntry]
[NodeDispatchGrid(1, 1, 1)] // This will be overridden during pipeline creation
[numthreads(8, 8, 1)]
void EntryFunction(uint3 DTid : SV_DispatchThreadID)
{
    RayDesc ray;
    ray = GenerateCameraRay(DTid.xy, CameraPosWS.xyz, InvViewProjection);
    // need to replace with viewport and width in constant buffer
    ray.TMin = 0.01;
    ray.TMax = 10000;
    
    RayQuery <  RAY_FLAG_FORCE_OPAQUE > rayQuery;
    rayQuery.TraceRayInline(Scene, RAY_FLAG_NONE, 0xFF, ray);
    rayQuery.Proceed();
   
    if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        float3 rayhit = ray.Origin + (ray.Direction * rayQuery.CommittedRayT());
        float3 normal = Normals[DTid.xy].xyz;

        RayDesc shadowRay;
        shadowRay.Origin = rayhit;
        shadowRay.Direction = normalize(lightPos.xyz - rayhit);
        shadowRay.TMin = 0.01;
        shadowRay.TMax = 10000;
        RayQuery < RAY_FLAG_FORCE_OPAQUE> ShadowRayQuery;
        ShadowRayQuery.TraceRayInline(Scene, RAY_FLAG_NONE, 0xFF, shadowRay);
        ShadowRayQuery.Proceed();
        if (ShadowRayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
        {

            float3 shadowRayHit = shadowRay.Origin + (shadowRay.Direction * ShadowRayQuery.CommittedRayT());

            float distance = length(shadowRay.Origin - shadowRayHit);
            float maxDistance = length(shadowRay.Origin - lightPos.xyz);
            if(distance > maxDistance)
            {
                RenderTarget[DTid.xy] *= CalculateDiffuseLighting(rayhit, normal);
            }
            else
            {
                RenderTarget[DTid.xy] = float4(0, 0, 0, 1.0);
            }
            
        }
        else
        {
           
            RenderTarget[DTid.xy] *= CalculateDiffuseLighting(rayhit, normal);
        }
    }
    else
    { 
        RenderTarget[DTid.xy] = float4(135, 206, 235, 1.0);
    }

    GroupMemoryBarrierWithGroupSync();

}

/*
[Shader("node")]
[NodeId("Worker", 0)]
[NodeLaunch("thread")]
void WorkerFunction()
{
    // Position cursor in center of screen.
    
    // Congratulations, you've successfully completed tutorial-0!
    // To move on to the next tutorial, open the "Tutorials" menu on the top-left of the playground application and select "Tutorial 1: Records".
}
*/