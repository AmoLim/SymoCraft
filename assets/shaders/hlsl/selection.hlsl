#include "draw_constants.hlsli"

float4 VSMain(float3 position : POSITION) : SV_POSITION
{
    return mul(model_view_projection, float4(position, 1.0f));
}

[earlydepthstencil]
float4 PSMain() : SV_TARGET
{
    return float4(0.863f, 0.078f, 0.235f, 1.0f);
}
