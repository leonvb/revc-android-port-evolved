uniform sampler2D tex0;
FSIN vec2 v_shadowTex;
void main(void)
{
    if(texture(tex0, vec2(v_shadowTex.x, 1.0-v_shadowTex.y)).a < 0.5) discard;
    // RGB8 carries enough precision for the bounded orthographic depth range.
    float depth = min(gl_FragCoord.z, 0.999999);
    vec3 encoded = fract(depth * vec3(1.0, 255.0, 65025.0));
    encoded -= encoded.yzz * vec3(1.0/255.0, 1.0/255.0, 0.0);
    FRAGCOLOR(vec4(encoded, 1.0));
}
