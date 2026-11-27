sampler2D tex0 : register(s0);
float4 fogColor : register(c0);
#include "modernLocal.h"
struct PS_INPUT {
    float4 Color : COLOR0;
    float3 Tex0 : TEXCOORD0;
    float3 WorldPos : TEXCOORD2;
    float3 WorldNormal : TEXCOORD3;
};
float4 main(PS_INPUT input) : COLOR
{
    float4 tex = tex2D(tex0, input.Tex0.xy);
    float4 color = input.Color * tex;
    color.rgb += tex.rgb * modernLocalLighting(input.WorldPos, input.WorldNormal);
    color.rgb = lerp(fogColor.rgb, color.rgb, input.Tex0.z);
    return color;
}
