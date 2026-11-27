#ifndef REVC_MODERN_LIGHTING_H
#define REVC_MODERN_LIGHTING_H

#include "ModernLightingPolicy.h"
#include "PointLights.h"

// Prepare bounded world-space uniforms for the current atomic. Fog-only and
// darkening lights are deliberately left to the classic renderer.
struct ModernLightUniforms {
	float position[ModernLightingPolicy::MaxLights][4];
	float color[ModernLightingPolicy::MaxLights][4];
	float direction[ModernLightingPolicy::MaxLights][4];
};
inline void BuildModernLights(rw::Atomic *atomic, bool enabled, ModernLightUniforms &out)
{
	memset(&out, 0, sizeof(out));
	if(!enabled || !atomic) return;
	ModernLightingPolicy::Light input[NUMPOINTLIGHTS];
	int count = 0;
	for(int i = 0; i < CPointLights::NumLights && i < NUMPOINTLIGHTS; ++i){
		const CRegisteredPointLight &p = CPointLights::aLights[i];
		if(p.type != CPointLights::LIGHT_POINT && p.type != CPointLights::LIGHT_DIRECTIONAL)
			continue;
		ModernLightingPolicy::Light &l = input[count++];
		l.x=p.coors.x; l.y=p.coors.y; l.z=p.coors.z; l.radius=p.radius;
		l.r=p.red; l.g=p.green; l.b=p.blue;
		l.dirX=p.dir.x; l.dirY=p.dir.y; l.dirZ=p.dir.z;
		l.directional = p.type == CPointLights::LIGHT_DIRECTIONAL;
	}
	const rw::Sphere *sphere = atomic->getWorldBoundingSphere();
	if(!sphere) return;
	int selected[ModernLightingPolicy::MaxLights];
	int num = ModernLightingPolicy::Select(input, count, sphere->center.x, sphere->center.y,
	                                       sphere->center.z, sphere->radius, selected);
	for(int i = 0; i < num; ++i){
		const ModernLightingPolicy::Light &l = input[selected[i]];
		out.position[i][0]=l.x; out.position[i][1]=l.y; out.position[i][2]=l.z; out.position[i][3]=l.radius;
		out.color[i][0]=Max(0.0f, l.r); out.color[i][1]=Max(0.0f, l.g); out.color[i][2]=Max(0.0f, l.b);
		out.direction[i][0]=l.dirX; out.direction[i][1]=l.dirY; out.direction[i][2]=l.dirZ;
		out.direction[i][3]=l.directional ? 1.0f : 0.0f;
	}
}
#endif
