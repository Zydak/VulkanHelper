struct VSInput {
	float3 position : POSITION;
	float3 color : COLOR;
};

struct VSOutput {
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

struct UBOData
{
	float4x4 MVP;
};

[[vk::binding(0)]] ConstantBuffer<UBOData> uboData[];

VSOutput main(VSInput input) 
{
	VSOutput output;

	UBOData ubo = uboData[1];

	// model * view * proj * pos
	output.position = mul(float4(input.position, 1.0f), ubo.MVP);

    //output.position = mul(push.transform, float4(input.position, 1.0));
	output.color = float4(input.color, 1.0);

	return output;
}