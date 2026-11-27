// Dual-Kawase bloom building block. This source is not wired into the game yet.
// Bounds are inclusive texel centers of the valid part of the source texture,
// in the source's native texture orientation. v_tex0 is already in that same
// orientation; each pass must supply its own orientation and bounds.
uniform sampler2D tex0;
uniform vec4 u_bloomBounds; // minU, minV, maxU, maxV
uniform vec4 u_bloomTexel;  // 1/texture width, 1/texture height, radius, unused
uniform vec4 u_bloomParams; // threshold, soft knee, pass (0=bright/down, 1=down, 2=up), unused
FSIN vec2 v_tex0;

vec3 bloomSample(vec2 uv)
{
    return texture(tex0, clamp(uv, u_bloomBounds.xy, u_bloomBounds.zw)).rgb;
}

vec3 bloomExtract(vec3 color)
{
    // A peak-channel threshold retains saturated red/blue neon in an LDR scene.
    float brightness = max(color.r, max(color.g, color.b));
    float knee = max(u_bloomParams.y, 0.00001);
    float soft = clamp(brightness - u_bloomParams.x + knee, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee);
    float contribution = max(soft, brightness - u_bloomParams.x) / max(brightness, 0.00001);
    return color * max(contribution, 0.0);
}

vec3 bloomDownSample(vec2 uv, bool extract)
{
    vec2 d = u_bloomTexel.xy * u_bloomTexel.z;
    vec3 a = bloomSample(uv);
    vec3 b = bloomSample(uv + vec2(-d.x, -d.y));
    vec3 c = bloomSample(uv + vec2( d.x, -d.y));
    vec3 e = bloomSample(uv + vec2(-d.x,  d.y));
    vec3 f = bloomSample(uv + vec2( d.x,  d.y));
    if (extract) {
        a = bloomExtract(a); b = bloomExtract(b); c = bloomExtract(c);
        e = bloomExtract(e); f = bloomExtract(f);
    }
    return (4.0 * a + b + c + e + f) / 8.0;
}

vec3 bloomUpSample(vec2 uv)
{
    vec2 d = u_bloomTexel.xy * u_bloomTexel.z;
    return (bloomSample(uv + vec2(-2.0*d.x, 0.0)) +
            bloomSample(uv + vec2( 2.0*d.x, 0.0)) +
            bloomSample(uv + vec2(0.0, -2.0*d.y)) +
            bloomSample(uv + vec2(0.0,  2.0*d.y)) +
            2.0 * (bloomSample(uv + vec2(-d.x, -d.y)) +
                   bloomSample(uv + vec2( d.x, -d.y)) +
                   bloomSample(uv + vec2(-d.x,  d.y)) +
                   bloomSample(uv + vec2( d.x,  d.y)))) / 12.0;
}

void main(void)
{
    vec3 color = u_bloomParams.z < 1.5
        ? bloomDownSample(v_tex0, u_bloomParams.z < 0.5)
        : bloomUpSample(v_tex0);
    FRAGCOLOR(vec4(color, 1.0));
}
