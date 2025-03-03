struct VSOutput {
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

float4 main(VSOutput input) : SV_Target {
    return pow(input.color, 1.0 / 2.2);
}