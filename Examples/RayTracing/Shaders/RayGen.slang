[[vk::binding(0, 0)]] RWTexture2D<float4> uImage;

[shader("raygeneration")]
void Main()
{
    uint3 size = DispatchRaysDimensions();
    uint3 LaunchID = DispatchRaysIndex();
    uImage[LaunchID.xy] = float4(LaunchID.xy / (float2)size.xy, 0.0f, 1.0f);
}