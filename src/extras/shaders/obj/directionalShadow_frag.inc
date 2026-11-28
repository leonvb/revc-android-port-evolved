const char *directionalShadow_frag_src =
"uniform sampler2D tex0;\n"
"FSIN vec2 v_shadowTex;\n"
"void main(void)\n"
"{\n"
"    if(texture(tex0, vec2(v_shadowTex.x, 1.0-v_shadowTex.y)).a < 0.5) discard;\n"
"    // RGB8 carries enough precision for the bounded orthographic depth range.\n"
"    float depth = min(gl_FragCoord.z, 0.999999);\n"
"    vec3 encoded = fract(depth * vec3(1.0, 255.0, 65025.0));\n"
"    encoded -= encoded.yzz * vec3(1.0/255.0, 1.0/255.0, 0.0);\n"
"    FRAGCOLOR(vec4(encoded, 1.0));\n"
"}\n"
;
