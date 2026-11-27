sampler2D tex0 : register(s0);
sampler2D tex1 : register(s1);
sampler2D tex2 : register(s2);
float4 fogColor : register(c0);
float4 lightMap : register(c1);
float4 modernPosition[8] : register(c10);
float4 modernColor[8] : register(c18);
float4 modernDirection[8] : register(c26);
float4 modernOptions : register(c34);
float4 modernSky : register(c35);
float4 modernEye : register(c36);
struct PS_INPUT {
    float4 Color : COLOR0;
    float3 Tex0 : TEXCOORD0;
    float2 Tex1 : TEXCOORD1;
    float3 WorldPos : TEXCOORD2;
    float3 WorldNormal : TEXCOORD3;
};
float4 main(PS_INPUT input) : COLOR
{
    float4 diffuse = tex2D(tex0, input.Tex0.xy);
    float4 lm = tex2D(tex1, input.Tex1);
    float4 color = diffuse * input.Color * (1.0 + lightMap * (lm - 1.0));
    color.a = input.Color.a * diffuse.a * lightMap.a;
    float3 N = input.WorldNormal * rsqrt(max(dot(input.WorldNormal, input.WorldNormal), 0.00001));
    float3 V = modernEye.xyz - input.WorldPos;
    V *= rsqrt(max(dot(V, V), 0.00001));
    float3 added = 0.0;
    float3 specular = 0.0;
    for(int i = 0; i < 8; ++i){
        float radius = modernPosition[i].w;
        if(radius <= 0.0) continue;
        float3 delta = modernPosition[i].xyz - input.WorldPos;
        float dist = length(delta);
        float3 L = delta / max(dist, 0.00001);
        float falloff = max(1.0 - dist / radius, 0.0);
        falloff *= falloff;
        if(modernDirection[i].w > 0.5){
            float3 dir = modernDirection[i].xyz;
            dir *= rsqrt(max(dot(dir, dir), 0.00001));
            falloff *= saturate((dot(-L, dir) - 0.5) * 2.0);
        }
        added += modernColor[i].rgb * falloff * max(dot(N, L), 0.0);
        float shine = pow(max(dot(reflect(-L, N), V), 0.0), 16.0);
        specular += modernColor[i].rgb * falloff * shine;
    }
    color.rgb += diffuse.rgb * added * 0.6;
    float wet = modernOptions.x * smoothstep(0.75, 0.95, N.z);
    if(wet > 0.0){
        float3 R = reflect(-V, N);
        float2 envUV = saturate(R.xy * 0.5 + 0.5);
        float3 env = modernSky.rgb;
        if(modernOptions.y > 0.5) env = tex2D(tex2, envUV).rgb;
        float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);
        color.rgb = lerp(color.rgb, env, wet * (0.08 + 0.24 * fresnel));
        color.rgb += specular * wet * 0.4;
    }
    color.rgb = lerp(fogColor.rgb, color.rgb, input.Tex0.z);
    return color;
}
