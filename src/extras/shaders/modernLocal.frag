uniform vec4 u_modernPosition[8];
uniform vec4 u_modernColor[8];
uniform vec4 u_modernDirection[8];
FSIN vec3 v_modernPos;
FSIN vec3 v_modernNormal;
vec3 modernLocalLighting()
{
    vec3 N = v_modernNormal * inversesqrt(max(dot(v_modernNormal, v_modernNormal), 0.00001));
    vec3 added = vec3(0.0);
    for(int i = 0; i < 8; ++i){
        float radius = u_modernPosition[i].w;
        if(radius <= 0.0) continue;
        vec3 delta = u_modernPosition[i].xyz - v_modernPos;
        float dist = length(delta);
        vec3 L = delta / max(dist, 0.00001);
        float falloff = max(1.0 - dist / radius, 0.0);
        falloff *= falloff;
        if(u_modernDirection[i].w > 0.5){
            vec3 dir = u_modernDirection[i].xyz;
            dir *= inversesqrt(max(dot(dir, dir), 0.00001));
            falloff *= clamp((dot(-L, dir) - 0.5) * 2.0, 0.0, 1.0);
        }
        added += u_modernColor[i].rgb * falloff * max(dot(N, L), 0.0);
    }
    return added * 0.6;
}
