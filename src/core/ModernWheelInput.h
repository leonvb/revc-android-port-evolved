#ifndef REVC_MODERN_WHEEL_INPUT_H
#define REVC_MODERN_WHEEL_INPUT_H
#include <stdint.h>
#include <math.h>

// Stateless with respect to the game world, allowing pause/cancel to reset it.
struct ModernWheelInput
{
	enum Event { NONE, TAP, SELECT };
	bool tracking, open;
	int button, selected;
	uint32_t started;

	void Reset() { tracking = open = false; button = selected = 0; started = 0; }
	Event Update(uint32_t now, bool left, bool right, float x, float y, int sectors)
	{
		if (!tracking) {
			if (!left && !right) return NONE;
			tracking = true;
			button = right ? 1 : -1;
			started = now;
			selected = -1;
		}
		bool held = button > 0 ? right : left;
		uint32_t duration = now - started;
		if (duration >= 180) open = true;
		if (open && sectors > 0 && x*x + y*y >= 25.0f*25.0f) {
			const float tau = 6.28318530718f;
			float a = atan2f(x, -y);
			if (a < 0.0f) a += tau;
			selected = (int)floorf(a / tau * sectors + 0.5f) % sectors;
		}
		if (!held) {
			Event event = open ? SELECT : TAP;
			tracking = open = false;
			return event;
		}
		return NONE;
	}
};
#endif
