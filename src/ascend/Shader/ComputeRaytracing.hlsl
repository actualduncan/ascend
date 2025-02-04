//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************
cbuffer RayTraceConstants : register(b0)
{
    //XMFLOAT4X4 InvViewProjection;
    float4x4 InvViewProjection;
    float4 CameraPosWS;
    float2 yes[22];
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

#ifndef COMPUTERAYTRACING_HLSL
#define COMPUTERAYTRACING_HLSL

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
    
    RayQuery < RAY_FLAG_CULL_NON_OPAQUE |
             RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
             RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH > rayQuery;
    rayQuery.TraceRayInline(Scene, RAY_FLAG_NONE, 0xFF, ray);
    rayQuery.Proceed();
   
    if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        
        RenderTarget[DTid.xy] = float4(1.0, 0, 0, 1.0);
    }
    else
    {
        RenderTarget[DTid.xy] = float4(0, 0, 0, 1.0);

    }

    GroupMemoryBarrierWithGroupSync();
}

#endif // COMPUTERAYTRACING_HLSL