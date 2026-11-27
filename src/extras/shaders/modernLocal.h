float4 modernPosition[8] : register(c10);
float4 modernColor[8] : register(c18);
float4 modernDirection[8] : register(c26);
float3 modernLocalLighting(float3 worldPos, float3 worldNormal)
{
    float3 N = worldNormal * rsqrt(max(dot(worldNormal, worldNormal), 0.00001));
    float3 added = 0.0;
    for(int i = 0; i < 8; ++i){
        float radius = modernPosition[i].w;
        if(radius <= 0.0) continue;
        float3 delta = modernPosition[i].xyz - worldPos;
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
    }
    return added * 0.6;
}
