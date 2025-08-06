struct VertexInput
{
    float2 Position;
    float2 Velocity; // Ignore for rendering, but needed for compute shader
};

struct VertexOutput
{
    float4 Position : SV_Position;
};

struct PushConstant
{
    float4x4 ModelViewProjection;
};

[[vk::push_constant]] PushConstant pushConstant;

[shader("vertex")]
VertexOutput Main(VertexInput input, out float gl_PointSize : SV_PointSize)
{
    VertexOutput output;
    output.Position = float4((input.Position / 50.0f) - 1.0f, 0.0f, 1.0f);
    gl_PointSize = 2.0f;
    return output;
}