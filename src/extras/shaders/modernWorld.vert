VSIN(ATTRIB_POS) vec3 in_pos;
uniform vec4 u_modernOptions; // wetness, env available, has vertex normals, unused
VSOUT vec4 v_color;
VSOUT vec2 v_tex0;
VSOUT vec2 v_tex1;
VSOUT float v_fog;
VSOUT vec3 v_worldPos;
VSOUT vec3 v_worldNormal;
void main(void)
{
    vec4 vertex = u_world * vec4(in_pos, 1.0);
    gl_Position = u_proj * u_view * vertex;
    vec3 normal = vec3(0.0);
    if(u_modernOptions.z > 0.5)
        normal = mat3(u_world) * in_normal;
    v_worldPos = vertex.xyz;
    v_worldNormal = normal;
    v_tex0 = in_tex0;
    v_tex1 = in_tex1;
    v_color = in_color;
    v_color.rgb += u_ambLight.rgb * surfAmbient;
    v_color.rgb += DoDynamicLight(vertex.xyz, normal) * surfDiffuse;
    v_color = clamp(v_color, 0.0, 1.0) * u_matColor;
    v_fog = DoFog(gl_Position.w);
}
