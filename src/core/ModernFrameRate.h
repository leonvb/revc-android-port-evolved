#ifndef REVC_MODERN_FRAME_RATE_H
#define REVC_MODERN_FRAME_RATE_H

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cctype>
#include <stdint.h>

// Engine-independent policy. Stored indices are stable: 0/1/2/3/4 mean
// 30/60/90/120/uncapped. Legacy Modern TargetFPS used the same indices.
namespace ModernFrameRate {
enum { FPS30 = 0, FPS60, FPS90, FPS120, UNCAPPED, COUNT };
inline bool Valid(int n) { return n >= FPS30 && n < COUNT; }
inline int Hertz(int n) {
	static const int hz[] = { 30, 60, 90, 120, 0 };
	return hz[Valid(n) ? n : FPS30];
}
inline int EffectiveHertz(int selection, bool menuActive) {
	return menuActive ? 30 : Hertz(selection);
}
inline bool ParseInteger(const char *text, int &out) {
	if (!text || !*text) return false;
	errno = 0;
	char *end = 0;
	long value = strtol(text, &end, 10);
	if (end == text || errno == ERANGE || value < INT_MIN || value > INT_MAX) return false;
	while (*end && std::isspace(static_cast<unsigned char>(*end))) ++end;
	if (*end) return false;
	out = static_cast<int>(value);
	return true;
}
inline int Migrate(const char *canonical, const char *limiter, const char *legacyTarget) {
	int value;
	if (ParseInteger(canonical, value) && Valid(value)) return value;
	if (ParseInteger(limiter, value) && value == 0) return UNCAPPED;
	if (ParseInteger(legacyTarget, value) && Valid(value)) return value;
	return FPS30;
}

// Deadline-based pacing avoids rounding 90/120 FPS to whole milliseconds.
// The caller supplies a monotonic clock in nanoseconds. A stall or clock reset
// starts a new schedule rather than trying to render a burst of missed frames.
struct Pacer {
	int previousHz;
	int64_t deadline;
	Pacer() : previousHz(0), deadline(0) {}
	bool Ready(int64_t now, int hz) {
		if (hz <= 0) { previousHz = 0; deadline = 0; return true; }
		const int64_t interval = 1000000000LL / hz;
		if (hz != previousHz || deadline <= 0 || now < 0 || now < deadline - interval * 2) {
			previousHz = hz;
			deadline = now + interval;
			return true;
		}
		if (now < deadline) return false;
		if (now - deadline >= interval) deadline = now + interval;
		else deadline += interval;
		return true;
	}
};
}
#endif
