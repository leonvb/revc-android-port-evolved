#include "../src/weapons/ModernAimGeometry.h"
#include <cassert>
#include <cmath>

struct Vec {
	float x, y, z;
	Vec operator-(const Vec &other) const { return { x-other.x, y-other.y, z-other.z }; }
	Vec operator*(float scalar) const { return { x*scalar, y*scalar, z*scalar }; }
};

static bool close(float a, float b) { return std::fabs(a-b) < 0.0001f; }

int main()
{
	const Vec muzzle = {1.0f, 0.0f, 0.0f};
	const Vec cameraDir = {0.0f, 1.0f, 0.0f};
	Vec target = {0.0f, 10.0f, 0.0f};
	Vec result = ModernMuzzleAimDirection(muzzle, target, cameraDir);
	assert(close(result.x, -1.0f / std::sqrt(101.0f)));
	assert(close(result.y, 10.0f / std::sqrt(101.0f)));
	assert(close(result.x*result.x + result.y*result.y + result.z*result.z, 1.0f));

	// The wall is behind or essentially at the muzzle: do not shoot backwards.
	result = ModernMuzzleAimDirection(muzzle, Vec{0.0f, -2.0f, 0.0f}, cameraDir);
	assert(close(result.x, 0.0f) && close(result.y, 1.0f));
	result = ModernMuzzleAimDirection(muzzle, muzzle, cameraDir);
	assert(close(result.x, 0.0f) && close(result.y, 1.0f));
	result = ModernMuzzleAimDirection(muzzle, Vec{2.0f, 0.01f, 0.0f}, cameraDir);
	assert(close(result.x, 0.0f) && close(result.y, 1.0f));
	result = ModernMuzzleAimDirection(muzzle, Vec{1.0f, 4.0f, 3.0f}, cameraDir);
	assert(close(result.y, 0.8f) && close(result.z, 0.6f));
}
