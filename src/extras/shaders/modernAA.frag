// Experimental contrast-adaptive edge smoothing, based on a local luma
// gradient. This is not the FXAA 3.11 Quality implementation.
uniform sampler2D tex0;
// xy = source texel size; zw = maximum valid UV before padding.
uniform vec4 u_modernAA;

FSIN vec2 v_tex0;

vec3 sampleScene(vec2 uv)
{
	vec2 lo = vec2(u_modernAA.x * 0.5, 1.0 - u_modernAA.w + u_modernAA.y * 0.5);
	vec2 hi = vec2(u_modernAA.z - u_modernAA.x * 0.5, 1.0 - u_modernAA.y * 0.5);
	return texture(tex0, clamp(uv, lo, hi)).rgb;
}

float luminance(vec3 color)
{
	return dot(color, vec3(0.299, 0.587, 0.114));
}

void main(void)
{
	vec2 uv = vec2(v_tex0.x, 1.0 - v_tex0.y);
	vec2 pixel = u_modernAA.xy;
	vec3 center = sampleScene(uv);
	float m = luminance(center);
	float nw = luminance(sampleScene(uv + vec2(-pixel.x, -pixel.y)));
	float ne = luminance(sampleScene(uv + vec2( pixel.x, -pixel.y)));
	float sw = luminance(sampleScene(uv + vec2(-pixel.x,  pixel.y)));
	float se = luminance(sampleScene(uv + vec2( pixel.x,  pixel.y)));
	float lo = min(m, min(min(nw, ne), min(sw, se)));
	float hi = max(m, max(max(nw, ne), max(sw, se)));

	if (hi - lo < max(0.0312, hi * 0.125)) {
		FRAGCOLOR(vec4(center, 1.0));
		return;
	}

	vec2 dir = vec2(-((nw + ne) - (sw + se)), (nw + sw) - (ne + se));
	float reduction = max((nw + ne + sw + se) * 0.03125, 0.0078125);
	dir = clamp(dir / (min(abs(dir.x), abs(dir.y)) + reduction), vec2(-8.0), vec2(8.0)) * pixel;
	vec3 inner = 0.5 * (sampleScene(uv - dir * (1.0 / 6.0)) + sampleScene(uv + dir * (1.0 / 6.0)));
	vec3 outer = inner * 0.5 + 0.25 * (sampleScene(uv - dir * 0.5) + sampleScene(uv + dir * 0.5));
	float outerLuma = luminance(outer);
	FRAGCOLOR(vec4((outerLuma < lo || outerLuma > hi) ? inner : outer, 1.0));
}
