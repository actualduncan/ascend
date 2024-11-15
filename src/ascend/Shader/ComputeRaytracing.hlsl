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

#ifndef COMPUTERAYTRACING_HLSL
#define COMPUTERAYTRACING_HLSL

RWTexture2D<float4> RenderTarget : register(u0);

[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint width;
    uint height;

    uint2 pixel = uint2(DTid.x, DTid.y);
    RenderTarget[pixel] = float4(1.0, 0, 0, 1.0);

    
    GroupMemoryBarrierWithGroupSync();
}

#endif // COMPUTERAYTRACING_HLSL