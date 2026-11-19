#ifdef __spirv__
struct CharacterConstants
{
    column_major float4x4 model_view_projection;
};
[[vk::push_constant]] ConstantBuffer<CharacterConstants> character_constants;
#define model_view_projection character_constants.model_view_projection
#else
#include "draw_constants.hlsli"
#endif

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 color : COLOR0;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 color : COLOR0;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(model_view_projection, float4(input.position, 1.0f));
    const float light = 0.4f + 0.6f * max(dot(input.normal, normalize(float3(0.4f, 1.0f, 0.7f))), 0.0f);
    output.color = input.color * light;
    return output;
}

float4 PSMain(VertexOutput input) : SV_TARGET
{
    return float4(input.color, 1.0f);
}
