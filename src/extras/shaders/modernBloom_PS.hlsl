// SM3 counterpart of modernBloom.frag. Not wired into the game yet.
sampler2D tex0 : register(s0);
float4 bloomBounds : register(c10); // minU, minV, maxU, maxV, inclusive texel centers
float4 bloomTexel  : register(c11); // reciprocal texture size, radius, unused
float4 bloomParams : register(c12); // threshold, soft knee, pass, unused
struct PS_INPUT {
    float4 position : POSITION;
    float3 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};
float3 bloomSample(float2 uv)
{
    return tex2D(tex0, clamp(uv, bloomBounds.xy, bloomBounds.zw)).rgb;
}
float3 bloomExtract(float3 color)
{
    // Match GLSL: preserve saturated red/blue highlights.
    float brightness = max(color.r, max(color.g, color.b));
    float knee = max(bloomParams.y, 0.00001);
    float soft = clamp(brightness - bloomParams.x + knee, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee);
    float contribution = max(soft, brightness - bloomParams.x) / max(brightness, 0.00001);
    return color * max(contribution, 0.0);
}
float3 bloomDownSample(float2 uv, bool extract)
{
    float2 d = bloomTexel.xy * bloomTexel.z;
    float3 a = bloomSample(uv);
    float3 b = bloomSample(uv + float2(-d.x, -d.y));
    float3 c = bloomSample(uv + float2( d.x, -d.y));
    float3 e = bloomSample(uv + float2(-d.x,  d.y));
    float3 f = bloomSample(uv + float2( d.x,  d.y));
    if (extract) {
        a = bloomExtract(a); b = bloomExtract(b); c = bloomExtract(c);
        e = bloomExtract(e); f = bloomExtract(f);
    }
    return (4.0 * a + b + c + e + f) / 8.0;
}
float3 bloomUpSample(float2 uv)
{
    float2 d = bloomTexel.xy * bloomTexel.z;
    return (bloomSample(uv + float2(-2.0*d.x, 0.0)) +
            bloomSample(uv + float2( 2.0*d.x, 0.0)) +
            bloomSample(uv + float2(0.0, -2.0*d.y)) +
            bloomSample(uv + float2(0.0,  2.0*d.y)) +
            2.0 * (bloomSample(uv + float2(-d.x, -d.y)) +
                   bloomSample(uv + float2( d.x, -d.y)) +
                   bloomSample(uv + float2(-d.x,  d.y)) +
                   bloomSample(uv + float2( d.x,  d.y)))) / 12.0;
}
float4 main(PS_INPUT input) : COLOR
{
    float3 color = bloomParams.z < 1.5
        ? bloomDownSample(input.texcoord.xy, bloomParams.z < 0.5)
        : bloomUpSample(input.texcoord.xy);
    return float4(color, 1.0);
}
