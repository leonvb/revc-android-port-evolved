#include "../src/core/ModernFrameRate.h"
#define MINI_CASE_SENSITIVE
#include "../src/extras/ini.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>

using namespace ModernFrameRate;

static int readMigrated(const mINI::INIStructure &cfg)
{
	auto graphics = cfg.get("Graphics");
	auto modern = cfg.get("Modern");
	const std::string canonical = graphics.has("FrameRate") ? graphics.get("FrameRate") : "";
	const std::string limiter = graphics.has("FrameLimiter") ? graphics.get("FrameLimiter") : "";
	const std::string legacy = modern.has("TargetFPS") ? modern.get("TargetFPS") : "";
	return Migrate(canonical.c_str(), limiter.c_str(), legacy.c_str());
}

int main()
{
	int n;
	assert(ParseInteger(" 4  ", n) && n == 4);
	for (const char *bad : { "", "abc", "1oops", "4.0", "9999999999999999999999999", "-9999999999999999999999999" })
		assert(!ParseInteger(bad, n));
	assert(!ParseInteger(nullptr, n));
	assert(Migrate(nullptr, nullptr, nullptr) == FPS30);
	assert(Migrate("2", "0", "4") == FPS90); // canonical has precedence
	assert(Migrate("bad", "0", "1") == UNCAPPED);
	assert(Migrate("99", "1", "3") == FPS120);
	assert(Migrate("-1", "1", "4") == UNCAPPED);
	assert(Migrate("", "1", "garbage") == FPS30);
	assert(Migrate("", "not-a-boolean", "") == FPS30);
	for (int i = 0; i < COUNT; ++i) {
		char text[4]; std::snprintf(text, sizeof(text), "%d", i);
		assert(Migrate(nullptr, "1", text) == i);
		assert(Migrate(text, "0", "4") == i);
	}
	assert(Hertz(FPS30) == 30 && Hertz(FPS60) == 60 && Hertz(FPS90) == 90 && Hertz(FPS120) == 120 && Hertz(UNCAPPED) == 0);
	assert(Hertz(999) == 30);
	for (int i = 0; i < COUNT; ++i) {
		assert(EffectiveHertz(i, false) == Hertz(i));
		assert(EffectiveHertz(i, true) == 30);
	}

	// Exercise real INI parser/write/restart and downgrade projections, preserving
	// unrelated keys and the independently configured VSync preference.
	const char *path = "/tmp/revc_modern_frame_rate_test.ini";
	{
		std::ofstream file(path);
		file << "[Graphics]\nFrameLimiter=0\nVSync=1\n[Modern]\nTargetFPS=3\n[Other]\nKeep=hello\n";
	}
	mINI::INIFile ini(path);
	mINI::INIStructure cfg;
	assert(ini.read(cfg));
	assert(readMigrated(cfg) == UNCAPPED);
	cfg["Graphics"]["FrameRate"] = "4";
	cfg["Graphics"]["FrameLimiter"] = "0";
	cfg["Modern"]["TargetFPS"] = "4";
	assert(ini.write(cfg));
	mINI::INIStructure restarted;
	assert(ini.read(restarted));
	assert(readMigrated(restarted) == UNCAPPED);
	assert(restarted.get("Graphics").get("VSync") == "1");
	assert(restarted.get("Other").get("Keep") == "hello");
	assert(Migrate(nullptr, restarted.get("Graphics").get("FrameLimiter").c_str(), restarted.get("Modern").get("TargetFPS").c_str()) == UNCAPPED);
	std::remove(path);

	for (int hz : { 30, 60, 90, 120 }) {
		Pacer pacer;
		int frames = 0;
		for (int64_t now = 0; now < 10000000000LL; now += 100000)
			if (pacer.Ready(now, hz)) ++frames;
		assert(frames == hz * 10); // ten seconds, no whole-millisecond drift
	}
	Pacer p;
	assert(p.Ready(1000000, 60));
	assert(!p.Ready(2000000, 60));
	assert(p.Ready(5000000000LL, 60)); // background/stall resynchronizes
	assert(!p.Ready(5000000001LL, 60)); // no burst of missed frames
	assert(p.Ready(5000000002LL, 0) && p.Ready(5000000003LL, 0)); // no cap
	assert(p.Ready(5000000004LL, 120)); // newly selected rate takes effect
	assert(!p.Ready(5000000005LL, 120));
	assert(p.Ready(1, 120)); // backwards clock reset
}
