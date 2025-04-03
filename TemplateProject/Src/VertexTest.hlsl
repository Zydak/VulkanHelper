struct VSInput {
	float3 position : POSITION;
	float3 color : COLOR;
};

struct VSOutput {
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

struct PushConstant {
	float4x4 MVP;
};

struct UBOData
{
	float4x4 MVP;
};

[[vk::binding(0)]] ConstantBuffer<UBOData> uboData[];

[[vk::push_constant]]
PushConstant push;

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