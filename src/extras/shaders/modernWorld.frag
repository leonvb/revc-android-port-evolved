uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform vec4 u_lightMap;
uniform vec3 u_eye;
uniform vec4 u_modernPosition[8]; // xyz world center, w radius (0 = disabled)
uniform vec4 u_modernColor[8];
uniform vec4 u_modernDirection[8]; // xyz direction, w directional flag
uniform vec4 u_modernOptions; // wetness, env available, unused, unused
uniform vec4 u_modernSky; // rgb sky fallback
FSIN vec4 v_color;
FSIN vec2 v_tex0;
FSIN vec2 v_tex1;
FSIN float v_fog;
FSIN vec3 v_worldPos;
FSIN vec3 v_worldNormal;
void main(void)
{
    vec4 diffuse = texture(tex0, vec2(v_tex0.x, 1.0-v_tex0.y));
    vec4 lm = texture(tex1, vec2(v_tex1.x, 1.0-v_tex1.y));
    vec4 color = diffuse * v_color * (1.0 + u_lightMap * (lm - 1.0));
    color.a = v_color.a * diffuse.a * u_lightMap.a;
    vec3 N = v_worldNormal * inversesqrt(max(dot(v_worldNormal, v_worldNormal), 0.00001));
    vec3 V = u_eye - v_worldPos;
    V *= inversesqrt(max(dot(V, V), 0.00001));
    vec3 added = vec3(0.0);
    vec3 specular = vec3(0.0);
    for(int i = 0; i < 8; ++i){
        float radius = u_modernPosition[i].w;
        if(radius <= 0.0) continue;
        vec3 delta = u_modernPosition[i].xyz - v_worldPos;
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
        float shine = pow(max(dot(reflect(-L, N), V), 0.0), 16.0);
        specular += u_modernColor[i].rgb * falloff * shine;
    }
    color.rgb += diffuse.rgb * added * 0.6;
    float wet = u_modernOptions.x * smoothstep(0.75, 0.95, N.z);
    if(wet > 0.0){
        vec3 R = reflect(-V, N);
        vec2 envUV = clamp(R.xy * 0.5 + 0.5, vec2(0.0), vec2(1.0));
        vec3 env = u_modernSky.rgb;
        if(u_modernOptions.y > 0.5) env = texture(tex2, vec2(envUV.x, 1.0-envUV.y)).rgb;
        float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);
        color.rgb = mix(color.rgb, env, wet * (0.08 + 0.24 * fresnel));
        color.rgb += specular * wet * 0.4;
    }
    color.rgb = mix(u_fogColor.rgb, color.rgb, v_fog);
    DoAlphaTest(color.a);
    FRAGCOLOR(color);
}
