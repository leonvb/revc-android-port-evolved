// FXAA 3.11 Quality, with bounded RGB-to-luma sampling of the scene copy.
#define FXAA_PC 1
#define FXAA_GLSL_130 1
#define FXAA_GATHER4_ALPHA 0
#define FXAA_QUALITY_PRESET 12
uniform sampler2D tex0;
uniform vec4 u_modernAA; // reciprocal texture dimensions, valid size in UVs
FSIN vec2 v_tex0;
vec4 FxaaSample(sampler2D tex, vec2 uv)
{
    vec2 lo = vec2(u_modernAA.x * 0.5, 1.0 - u_modernAA.w + u_modernAA.y * 0.5);
    vec2 hi = vec2(u_modernAA.z - u_modernAA.x * 0.5, 1.0 - u_modernAA.y * 0.5);
    vec3 rgb = textureLod(tex, clamp(uv, lo, hi), 0.0).rgb;
    return vec4(rgb, dot(rgb, vec3(0.299, 0.587, 0.114)));
}
#include "fxaa311.h"
void main(void)
{
    vec2 uv = vec2(v_tex0.x, 1.0 - v_tex0.y);
    vec4 result = FxaaPixelShader(uv, vec4(0.0), tex0, tex0, tex0,
        u_modernAA.xy, vec4(0.0), vec4(0.0), vec4(0.0),
        0.75, 0.166, 0.0312, 0.0, 0.0, 0.0, vec4(0.0));
    FRAGCOLOR(vec4(result.rgb, 1.0));
}
