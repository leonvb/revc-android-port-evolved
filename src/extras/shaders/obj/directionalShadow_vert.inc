const char *directionalShadow_vert_src =
"VSIN(ATTRIB_POS) vec3 in_pos;\n"
"uniform mat4 u_shadowMatrix;\n"
"VSOUT vec2 v_shadowTex;\n"
"void main(void)\n"
"{\n"
"    gl_Position = u_shadowMatrix * u_world * vec4(in_pos, 1.0);\n"
"    v_shadowTex = in_tex0;\n"
"}\n"
;
