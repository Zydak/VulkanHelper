struct VertexInput
{
    float2 Position;
    float2 Velocity;
};

struct VertexOutput
{
    float4 Position : SV_Position;
    float3 Color;
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

    // Color based on velocity
    float speed = length(input.Velocity);
    //speed = clamp(speed / 10.0f, 0.0f, 1.0f);
    output.Color = lerp(float3(0.85f, 0.0f, 0.0f), float3(0.3f, 0.0f, 0.85f), speed);
    return output;
}