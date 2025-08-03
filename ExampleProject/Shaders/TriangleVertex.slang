struct VertexOutput
{
    float4 Position : SV_Position;
    float3 Color;
};

static const float4 s_Positions[3] = {
    float4( 0.0, -0.5, 0.0, 1.0),  // Top
    float4(-0.5,  0.5, 0.0, 1.0),  // Bottom left
    float4( 0.5,  0.5, 0.0, 1.0)   // Bottom right
};

static const float3 s_Colors[3] = {
    float3(0.0, 0.0, 1.0),  // Top
    float3(0.0, 1.0, 0.0),  // Bottom left
    float3(1.0, 0.0, 0.0)   // Bottom right
};

// Vertex shader entry point
[shader("vertex")]
VertexOutput Main(uint vertexID : SV_VertexID)
{
    VertexOutput output;
    output.Position = s_Positions[vertexID];
    output.Color = s_Colors[vertexID];
    return output;
}