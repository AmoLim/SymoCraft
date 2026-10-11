#ifndef SYMOCRAFT_DRAW_CONSTANTS_HLSLI
#define SYMOCRAFT_DRAW_CONSTANTS_HLSLI

cbuffer DrawConstants : register(b0)
{
    column_major float4x4 model_view_projection;
};

#endif
