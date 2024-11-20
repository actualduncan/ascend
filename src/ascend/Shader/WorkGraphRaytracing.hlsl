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

RWTexture2D<float4> RenderTarget : register(u0);
RaytracingAccelerationStructure Scene : register(t0, space0);

[Shader("node")]
[NodeLaunch("broadcasting")]
[NodeIsProgramEntry]
[NodeDispatchGrid(1, 1, 1)] // This will be overridden during pipeline creation
[numthreads(1, 1, 1)]
void EntryFunction(uint3 DTid : SV_DispatchThreadID)
{
    float3 dtfloat = float3(DTid.xyz);
    uint2 pixel = uint2(DTid.x, DTid.y);
    RayDesc ray;

    // need to replace with viewport and width in constant buffer
    ray.Origin = float3(lerp(-1.0, 1.0, dtfloat.x / 1280.0), lerp(-1.0, 1.0, dtfloat.y / 800.0), 0.0);
    ray.Direction = float3(0, 0, 1);
    ray.TMin = 0.01;
    ray.TMax = 100000;
    
    RayQuery < RAY_FLAG_CULL_NON_OPAQUE |
             RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
             RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH > rayQuery;
    rayQuery.TraceRayInline(Scene, RAY_FLAG_NONE, 0xFF, ray);
    rayQuery.Proceed();
   
    if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        
        RenderTarget[pixel] = float4(1.0, 0, 0, 1.0);
    }
    else
    {
        RenderTarget[pixel] = float4(0, 0, 0, 1.0);

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