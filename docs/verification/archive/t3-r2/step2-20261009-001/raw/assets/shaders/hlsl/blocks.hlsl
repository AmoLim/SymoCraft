#include "draw_constants.hlsli"

struct VertexInput
{
    int3 position : POSITION;
    float3 texture_coordinate : TEXCOORD0;
    float direction : DIRECTION;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 texture_coordinate : TEXCOORD0;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(model_view_projection, float4(input.position, 1.0f));
    output.texture_coordinate = input.texture_coordinate;
    return output;
}

Texture2DArray<float4> texture_array : register(t0);
SamplerState point_wrap_sampler : register(s0);

float4 PSMain(VertexOutput input) : SV_TARGET
{
    float4 color = texture_array.Sample(point_wrap_sampler, input.texture_coordinate);
    if (color.a < 0.01f)
        discard;
    return color;
}
