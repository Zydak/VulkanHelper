struct VertexInput
{
    float3 Color;
};

[shader("pixel")]
float4 Main(VertexInput vertexInput) : SV_Target
{
    float3 Color = vertexInput.Color;
    // Convert to SRGB
    Color = pow(Color, 1.0 / 2.2);
    return float4(Color, 1);
}