struct Payload
{
    float4 Color;
    bool RenderNormalsOrTexCoords;
};

[shader("miss")]
void Main(inout Payload payload)
{
    payload.Color = float4(0.1f, 0.1f, 0.1f, 1.0f);
}