#include "../src/core/ModernPedalInput.h"
#include <cassert>
#include <cmath>
#include <initializer_list>
#include <limits>

int main()
{
	using namespace ModernPedalInput;
	assert(Pedal(0.0f) == 0);
	assert(Pedal(0.03f) == 0);
	assert(Pedal(1.0f) == 255);
	assert(Pedal(-1.0f) == 0);
	assert(Pedal(2.0f) == 255);
	assert(Pedal(std::numeric_limits<float>::quiet_NaN()) == 0);
	int previous = 0;
	for (float fraction : {0.0f, 0.25f, 0.50f, 0.75f, 1.0f}) {
		int value = Pedal(fraction);
		assert(value >= previous);
		assert(value >= 0 && value <= 255);
		previous = value;
	}
	assert(Pedal(0.5f) >= 120 && Pedal(0.5f) <= 128);
	// SDL uses 0..32767; GLFW uses -1..1; XInput uses 0..255.
	float sdl = 16384.0f / 32767.0f;
	float glfw = (0.0f + 1.0f) * 0.5f;
	float xinput = 128.0f / 255.0f;
	assert(std::abs(Pedal(sdl) - Pedal(glfw)) <= 1);
	assert(std::abs(Pedal(xinput) - Pedal(glfw)) <= 1);
	assert(Combine(Pedal(0.5f), 0, 0) == Pedal(0.5f));
	assert(Combine(Pedal(0.5f), 255, 0) == 255);
	assert(Combine(Pedal(0.5f), 0, 255) == 255);
	// Throttle and brake remain independent when both are pressed.
	assert(Pedal(0.25f) != 0 && Pedal(0.75f) > Pedal(0.25f));
}
