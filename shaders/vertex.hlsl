struct VSOut
{
    float4 pos : SV_POSITION;
    float3 col : COLOR;
};

VSOut main(float3 pos : POSITION, float3 col : COLOR)
{
    VSOut output;
    output.pos = float4(pos, 1.0f);
    output.col = col;
    return output;
}