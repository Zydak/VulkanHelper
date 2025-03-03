struct VSOutput {
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

VSOutput main(uint id : SV_VertexID) 
{
	VSOutput output;

	float2 vertices[3] = {
		float2(0.0, -0.8),  // Top
		float2(0.8,  0.8),  // Bottom Right
		float2(-0.8, 0.8)  // Bottom Left
	};

	float3 colors[3] = {
		float3(1.0, 0.0, 0.0),
		float3(0.0, 1.0, 0.0),
		float3(0.0, 0.0, 1.0)
	};

	output.position = float4(vertices[id], 0.0, 1.0);
	output.color = float4(colors[id], 1.0);

	return output;
}