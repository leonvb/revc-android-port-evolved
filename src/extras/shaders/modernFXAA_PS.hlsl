// FXAA 3.11 Quality, with bounded RGB-to-luma sampling of the scene copy.
#define FXAA_PC 1
#define FXAA_HLSL_3 1
#define FXAA_GATHER4_ALPHA 0
#define FXAA_QUALITY_PRESET 12
sampler2D tex0 : register(s0);
float4 modernAA : register(c10);
struct PS_INPUT {
    float4 position : POSITION;
    float3 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};
float4 FxaaSample(sampler2D tex, float2 uv)
{
    float2 lo = modernAA.xy * 0.5;
    float2 hi = modernAA.zw - modernAA.xy * 0.5;
    float3 rgb = tex2Dlod(tex, float4(clamp(uv, lo, hi), 0.0, 0.0)).rgb;
    return float4(rgb, dot(rgb, float3(0.299, 0.587, 0.114)));
}
#include "fxaa311.h"
float4 main(PS_INPUT input) : COLOR
{
    float4 result = FxaaPixelShader(input.texcoord.xy, 0.0, tex0, tex0, tex0,
        modernAA.xy, 0.0, 0.0, 0.0, 0.75, 0.166, 0.0312, 0.0, 0.0, 0.0, 0.0);
    return float4(result.rgb, 1.0);
}
