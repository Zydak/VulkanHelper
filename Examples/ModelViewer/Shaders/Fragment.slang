struct VertexInput
{
    float3 Normal;
    float2 TexCoord;
};

[[vk::binding(0, 0)]] Texture2D uTexture;
[[vk::binding(1, 0)]] SamplerState uSampler;


[shader("pixel")]
float4 Main(VertexInput vertexInput) : SV_Target
{
    float3 Color = uTexture.Sample(uSampler, vertexInput.TexCoord).rgb;
    return float4(Color, 1);
}