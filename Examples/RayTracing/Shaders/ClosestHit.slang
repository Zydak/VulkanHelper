struct Vertex
{
    float3 Position;
    float3 Normal;
    float2 TexCoord;
};

[[vk::binding(3, 0)]] StructuredBuffer<Vertex, ScalarDataLayout> uVertices;
[[vk::binding(4, 0)]] StructuredBuffer<uint> uIndices;

struct Payload
{
    float4 Color;
    bool RenderNormalsOrTexCoords;
};

[shader("closesthit")]
void Main(inout Payload payload, in float2 attrib)
{
    float3 barycentrics = float3(1.0f - attrib.x - attrib.y, attrib.x, attrib.y);
    uint primitiveIndex = PrimitiveIndex();

    const uint i1 = uIndices[primitiveIndex * 3 + 0];
    const uint i2 = uIndices[primitiveIndex * 3 + 1];
    const uint i3 = uIndices[primitiveIndex * 3 + 2];

    const Vertex v1 = uVertices[i1];
    const Vertex v2 = uVertices[i2];
    const Vertex v3 = uVertices[i3];

    float3 position = v1.Position * barycentrics.x + v2.Position * barycentrics.y + v3.Position * barycentrics.z;
    position = mul(ObjectToWorld3x4(), float4(position, 1.0f)).xyz;

    float3 normal = normalize(v1.Normal * barycentrics.x + v2.Normal * barycentrics.y + v3.Normal * barycentrics.z);
    normal = mul(normal, WorldToObject()).xyz;

    const float2 texCoord = v1.TexCoord * barycentrics.x + v2.TexCoord * barycentrics.y + v3.TexCoord * barycentrics.z;

    const float3 color = float3(texCoord, 0.0f) * (float)payload.RenderNormalsOrTexCoords + (normal + 1.0f) / 2.0f * (1.0f - (float)payload.RenderNormalsOrTexCoords);

    payload.Color = float4(color, 1.0f);
}