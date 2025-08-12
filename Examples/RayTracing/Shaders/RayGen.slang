[[vk::image_format("rgba8")]]
[[vk::binding(0, 0)]] RWTexture2D<float4> uImage;

[[vk::binding(1, 0)]] RaytracingAccelerationStructure uTopLevelAS;

struct UniformBuffer
{
    float4x4 ViewInverse;
    float4x4 ProjectionInverse;
    bool RenderNormalsOrTexCoords;
};

[[vk::binding(2, 0)]] ConstantBuffer<UniformBuffer> uUBO;

struct Payload
{
    float4 Color;
    bool RenderNormalsOrTexCoords;
};

[shader("raygeneration")]
void Main()
{
    uint3 size = DispatchRaysDimensions();
    uint3 LaunchID = DispatchRaysIndex();

    const float2 pixelCenter = float2(LaunchID.xy) + float2(0.5f);
    const float2 pixelUV = pixelCenter / float2(size.xy);
    const float2 d = pixelUV * 2.0f - 1.0f;

    float4 origin       = mul(uUBO.ViewInverse, float4(0, 0, 0, 1));
    float4 target       = mul(uUBO.ProjectionInverse, float4(d.x, d.y, 1.0f, 1.0f));
    float4 direction    = mul(uUBO.ViewInverse, float4(normalize(target.xyz), 0.0f));

    RayDesc rayDesc;
    rayDesc.Origin = origin.xyz;
    rayDesc.Direction = direction.xyz;
    rayDesc.TMin = 0.0f;
    rayDesc.TMax = 1000.0f;

    Payload payload;
    payload.Color = float4(0.0f, 0.0f, 0.0f, 0.0f);
    payload.RenderNormalsOrTexCoords = uUBO.RenderNormalsOrTexCoords;

    TraceRay(uTopLevelAS, RAY_FLAG_FORCE_OPAQUE, 0xff, 0u, 0u, 0u, rayDesc, payload);

    uImage[LaunchID.xy] = payload.Color;
}