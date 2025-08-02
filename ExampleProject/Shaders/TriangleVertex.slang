// Define the output structure
struct VertexOutput
{
    float4 position : SV_Position;
};

// Hardcoded triangle vertex positions in clip space
static const float4 positions[3] = {
    float4( 0.0,  0.5, 0.0, 1.0),  // Top
    float4(-0.5, -0.5, 0.0, 1.0),  // Bottom left
    float4( 0.5, -0.5, 0.0, 1.0)   // Bottom right
};

// Vertex shader entry point
[shader("vertex")]
VertexOutput Main(uint vertexID : SV_VertexID)
{
    VertexOutput output;
    output.position = positions[vertexID];
    return output;
}