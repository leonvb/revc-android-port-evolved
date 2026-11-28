#ifndef REVC_MODERN_AIM_GEOMETRY_H
#define REVC_MODERN_AIM_GEOMETRY_H

#include <math.h>

// Convert a camera-ray hit point to a muzzle ray. A surface between the camera
// and muzzle is behind the barrel; never point the weapon backwards toward it.
// Vec supplies x/y/z, subtraction and multiplication by a scalar.
template<class Vec>
Vec ModernMuzzleAimDirection(const Vec &muzzle, const Vec &hit, const Vec &cameraDirection)
{
	Vec delta = hit - muzzle;
	const float along = delta.x * cameraDirection.x + delta.y * cameraDirection.y + delta.z * cameraDirection.z;
	const float lengthSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
	if (!(along > 0.05f) || !(lengthSq > 0.0001f))
		return cameraDirection;
	return delta * (1.0f / sqrtf(lengthSq));
}

#endif
