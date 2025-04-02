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

[[vk::push_constant]]
PushConstant push;

VSOutput main(VSInput input) 
{
	VSOutput output;

	// model * view * proj * pos
	output.position = mul(float4(input.position, 1.0f), push.MVP);

    //output.position = mul(push.transform, float4(input.position, 1.0));
	output.color = float4(input.color, 1.0);

	return output;
}