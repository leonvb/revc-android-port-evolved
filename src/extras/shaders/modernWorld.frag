uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform mat4 u_shadowMatrix;
uniform vec4 u_shadowParams; // strength, inverse texture size, depth bias, enabled
uniform vec4 u_lightMap;
uniform vec3 u_eye;
uniform vec4 u_modernPosition[8]; // xyz world center, w radius (0 = disabled)
uniform vec4 u_modernColor[8];
uniform vec4 u_modernDirection[8]; // xyz direction, w directional flag
uniform vec4 u_modernOptions; // wetness, env available, has vertex normals, unused
uniform vec4 u_modernSky; // rgb sky fallback
FSIN vec4 v_color;
FSIN vec2 v_tex0;
FSIN vec2 v_tex1;
FSIN float v_fog;
FSIN vec3 v_worldPos;
FSIN vec3 v_worldNormal;
float shadowVisibility(vec3 worldPos)
{
    if(u_shadowParams.w < 0.5) return 1.0;
    vec3 p = (u_shadowMatrix * vec4(worldPos, 1.0)).xyz * 0.5 + 0.5;
    if(p.z <= 0.0 || p.z >= 1.0 || p.x <= 0.0 || p.x >= 1.0 || p.y <= 0.0 || p.y >= 1.0)
        return 1.0;
    float sum = 0.0;
    for(int y = -1; y <= 1; ++y){
        for(int x = -1; x <= 1; ++x){
            vec2 uv = clamp(p.xy + vec2(float(x), float(y)) * u_shadowParams.y,
                            vec2(0.0), vec2(1.0));
            vec3 encodedDepth = texture(tex3, uv).rgb;
            float depth = dot(encodedDepth, vec3(1.0, 1.0/255.0, 1.0/65025.0));
            sum += p.z - u_shadowParams.z <= depth ? 1.0 : 0.0;
        }
    }
    float edge = smoothstep(0.0, 0.04, min(min(p.x, 1.0-p.x), min(p.y, 1.0-p.y)));
    return 1.0 - u_shadowParams.x * edge * (1.0 - sum/9.0);
}
void main(void)
{
    vec4 diffuse = texture(tex0, vec2(v_tex0.x, 1.0-v_tex0.y));
    vec4 lm = texture(tex1, vec2(v_tex1.x, 1.0-v_tex1.y));
    vec4 color = diffuse * v_color * (1.0 + u_lightMap * (lm - 1.0));
    color.a = v_color.a * diffuse.a * u_lightMap.a;
    color.rgb *= shadowVisibility(v_worldPos);
    vec3 normal = v_worldNormal;
    if(u_modernOptions.z < 0.5){
#ifndef GL2
        // GTA world meshes often have prelit colours but no vertex normals.
        // Use the triangle's geometric normal for local lights and wet roads.
        normal = cross(dFdx(v_worldPos), dFdy(v_worldPos));
        if(!gl_FrontFacing) normal = -normal;
#else
        // GLES 2 derivatives are optional; leave local lighting disabled.
        normal = vec3(0.0);
#endif
    }
    vec3 N = normal * inversesqrt(max(dot(normal, normal), 0.00001));
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
