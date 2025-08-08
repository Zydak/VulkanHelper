struct VertexInput
{
    float3 Position;
    float3 Normal;
    float2 TexCoord;
};

struct VertexOutput
{
    float4 Position : SV_Position;
    float3 Normal;
    float2 TexCoord;
};

struct PushConstants
{
    float4x4 MVP;
};

[[vk::push_constant]] PushConstants pushConstants;

[shader("vertex")]
VertexOutput Main(VertexInput input)
{
    VertexOutput output;
    output.Position = mul(pushConstants.MVP, float4(input.Position, 1.0));
    output.Normal = input.Normal;
    output.TexCoord = input.TexCoord;
    return output;
}