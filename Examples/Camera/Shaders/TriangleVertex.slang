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

struct PushConstantData
{
    float3 Color;
};

[[vk::push_constant]]
PushConstantData pushConstantData;

struct SomeData
{
    float3 Color;
};

[[vk::binding(0, 0)]] ConstantBuffer<SomeData> uSomeData;

[shader("vertex")]
VertexOutput Main(VertexInput input)
{
    VertexOutput output;
    output.Position = float4(input.Position, 1.0);
    output.Color = uSomeData.Color * input.Color;
    return output;
}