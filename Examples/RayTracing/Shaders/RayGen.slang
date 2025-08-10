[[vk::binding(0, 0)]] RWTexture2D<float4> uImage;

[shader("raygeneration")]
void Main()
{
    uint3 LaunchID = DispatchRaysIndex();
    uImage[LaunchID.xy] = float4(LaunchID.xy / 900.0f, 0.0f, 1.0f);
}