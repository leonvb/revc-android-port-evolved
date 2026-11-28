VSIN(ATTRIB_POS) vec3 in_pos;
uniform mat4 u_shadowMatrix;
VSOUT vec2 v_shadowTex;
void main(void)
{
    gl_Position = u_shadowMatrix * u_world * vec4(in_pos, 1.0);
    v_shadowTex = in_tex0;
}
