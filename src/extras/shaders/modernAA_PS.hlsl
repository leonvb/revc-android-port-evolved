// Experimental contrast-adaptive edge smoothing, not FXAA 3.11 Quality.
sampler2D tex0 : register(s0);
// xy = source texel size; zw = maximum valid UV before padding.
float4 modernAA : register(c10);

struct PS_INPUT {
    float4 position : POSITION;
    float3 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

float3 sampleScene(float2 uv)
{
    float2 lo = modernAA.xy * 0.5;
    float2 hi = modernAA.zw - modernAA.xy * 0.5;
    return tex2D(tex0, clamp(uv, lo, hi)).rgb;
}

float luminance(float3 color)
{
    return dot(color, float3(0.299, 0.587, 0.114));
}

float4 main(PS_INPUT input) : COLOR
{
    float2 uv = input.texcoord.xy;
    float2 pixel = modernAA.xy;
    float3 center = sampleScene(uv);
    float m = luminance(center);
    float nw = luminance(sampleScene(uv + float2(-pixel.x, -pixel.y)));
    float ne = luminance(sampleScene(uv + float2( pixel.x, -pixel.y)));
    float sw = luminance(sampleScene(uv + float2(-pixel.x,  pixel.y)));
    float se = luminance(sampleScene(uv + float2( pixel.x,  pixel.y)));
    float lo = min(m, min(min(nw, ne), min(sw, se)));
    float hi = max(m, max(max(nw, ne), max(sw, se)));
    if (hi - lo < max(0.0312, hi * 0.125))
        return float4(center, 1.0);

    float2 dir = float2(-((nw + ne) - (sw + se)), (nw + sw) - (ne + se));
    float reduction = max((nw + ne + sw + se) * 0.03125, 0.0078125);
    dir = clamp(dir / (min(abs(dir.x), abs(dir.y)) + reduction), -8.0, 8.0) * pixel;
    float3 inner = 0.5 * (sampleScene(uv - dir * (1.0 / 6.0)) + sampleScene(uv + dir * (1.0 / 6.0)));
    float3 outer = inner * 0.5 + 0.25 * (sampleScene(uv - dir * 0.5) + sampleScene(uv + dir * 0.5));
    float outerLuma = luminance(outer);
    return float4((outerLuma < lo || outerLuma > hi) ? inner : outer, 1.0);
}
