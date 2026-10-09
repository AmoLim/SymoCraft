Texture2DArray<float4> fixtureTexture : register(t0);
SamplerState fixtureSampler : register(s0);

struct VertexInput {
    int3 position : POSITION;
    float3 uvLayer : TEXCOORD0;
    float normal : NORMAL;
};
struct PixelInput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    nointerpolation uint layer : TEXCOORD1;
};

PixelInput VSMain(VertexInput input) {
    PixelInput result;
    // The y-down 4x4 fixture is not the production world/camera convention.
    result.position = float4(float(input.position.x) * 0.5 - 1.0,
                             1.0 - float(input.position.y) * 0.5, 0.5, 1.0);
    result.uv = input.uvLayer.xy;
    result.layer = uint(input.uvLayer.z);
    return result;
}

float4 PSMain(PixelInput input) : SV_Target {
    float4 sampleValue = fixtureTexture.Sample(fixtureSampler, float3(input.uv, input.layer));
    clip(sampleValue.a - 0.01);
    return sampleValue;
}
