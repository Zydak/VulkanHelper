struct VertexInput
{
    float3 Position;
    float3 Color;
};

struct VertexOutput
{
    float4 Position : SV_Position;
    float3 Color;
};

// Vertex shader entry point
[shader("vertex")]
VertexOutput Main(VertexInput input)
{
    VertexOutput output;
    output.Position = float4(input.Position, 1.0);
    output.Color = input.Color;
    return output;
}