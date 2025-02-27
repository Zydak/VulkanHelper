struct VSOutput {
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

// Vertex Shader
VSOutput main(uint id : SV_VertexID) 
{
	VSOutput output;

	// Hardcoded triangle vertices in NDC space
	float2 vertices[3] = {
		float2(0.0,  0.5),  // Top
		float2(0.5, -0.5),  // Bottom Right
		float2(-0.5, -0.5)  // Bottom Left
	};

	output.position = float4(vertices[id], 0.0, 1.0);
	output.color = float4(1.0, 0.0, 0.0, 1.0); // Red color

	return output;
}