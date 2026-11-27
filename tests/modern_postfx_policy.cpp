#include "../src/extras/ModernPostFXPolicy.h"
#include <cassert>

int main()
{
	float p[4] = {};
	assert(!ModernPostFXPolicy::CaptureParams(0, 720, 1024, 1024, p));
	assert(!ModernPostFXPolicy::CaptureParams(1280, 720, 1024, 1024, p));
	assert(!ModernPostFXPolicy::CaptureParams(1280, 720, 2048, 0, p));
	assert(!ModernPostFXPolicy::CaptureParams(1280, 720, 2048, 1024, 0));
	assert(ModernPostFXPolicy::CaptureParams(1280, 720, 2048, 1024, p));
	assert(p[0] == 1.0f / 2048.0f);
	assert(p[1] == 1.0f / 1024.0f);
	assert(p[2] == 1280.0f / 2048.0f);
	assert(p[3] == 720.0f / 1024.0f);
	assert(ModernPostFXPolicy::CaptureParams(1, 1, 1, 1, p));
	assert(p[0] == 1.0f && p[1] == 1.0f && p[2] == 1.0f && p[3] == 1.0f);
}
