struct PSInput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD;
};

Texture2D FontTexture : register(t0);
sampler FontSampler : register(s0);

float4 PSMain(PSInput input) : SV_Target
{
    float4 outputColor = input.color * FontTexture.Sample(FontSampler, input.uv);
    return outputColor;
}