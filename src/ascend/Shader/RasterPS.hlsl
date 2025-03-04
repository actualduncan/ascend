
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
    float3 tangent : TANGENT;
    float3 bitTangent : BITTANGENT;
    float2 uv : TEXCOORD;
};
struct PSOutput
{
    float4 Color : SV_TARGET0;
    float4 Normal : SV_Target1;
    float4 WorldSpace : SV_Target2;
    float4 Material : SV_Target3;
};
float magnitude(float3 _vector)
{
    return sqrt(pow(_vector.x, 2) + pow(_vector.y, 2) + pow(_vector.z, 2)); // find magnitude of any vector (created cause no magnitude() function in hlsl)

}

float3 recalculateNormals(float3 currentNormal, float3 bumpMap)
{
    // calculates each tangent based on forward/up vector and current normal
    float3 tangent1 = cross(currentNormal, float3(0, 0, 1)); // forward vector
    float3 tangent2 = cross(currentNormal, float3(0, 1, 0)); // up vector
    
    // sets tangent to the tangent with the largest magnitude
    float3 tangent;
    if (magnitude(tangent1) > magnitude(tangent2))
    {
        tangent = tangent1;
    }
    else
    {
        tangent = tangent2;
    }
    

    // Expand the range of the normal value from (0, +1) to (-1, +1).
    bumpMap = (bumpMap * 2.0f) - 1.0f;
 
    // Calculate the normal from the data in the bump map.
    float3 N = currentNormal;
    float3 T = normalize(tangent - dot(tangent, N) * N); // tangent
    float3 B = cross(N, T); // bitangent
   
    float3x3 TBN = float3x3(T, B, N); // matrix used to transform bump map from tangent space to world space
    
    return mul(bumpMap, TBN);
}

Texture2D g_texture : register(t0);
Texture2D g_normtexture : register(t1);
SamplerState g_sampler : register(s0);

PSOutput PSMain(PSInput input)  
{
    float4 bumpMap = g_normtexture.Sample(g_sampler, input.uv);
    PSOutput output;
    output.Color = g_texture.Sample(g_sampler, input.uv) * float4(1, 1, 1, 1); //Set first output
    output.Normal = float4(recalculateNormals(input.normal.xyz, bumpMap.xyz), 1.0f); //Set second output
    output.WorldSpace = float4(abs(input.tangent.xyz), 1.0f);
    output.Material = float4(0, 0, 0, 0);
    return output;
}
