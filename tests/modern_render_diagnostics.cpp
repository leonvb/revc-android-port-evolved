#define ANDROID
#include "../src/extras/ModernRenderDiagnostics.h"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

static std::vector<std::string> logLines;
void Logger::Log(const char *format, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    logLines.push_back(buffer);
}

int main()
{
    using namespace ModernRenderDiagnostics;
    OncePerState tracker;
    assert(tracker.ShouldReport(Bloom, Disabled));
    assert(!tracker.ShouldReport(Bloom, Disabled));
    assert(tracker.ShouldReport(Bloom, Submitted));
    assert(!tracker.ShouldReport(Bloom, Submitted));
    assert(tracker.ShouldReport(Fxaa, Disabled));
    assert(tracker.ShouldReport(Shadow, NoCasters));
    assert(tracker.ShouldReport(Shadow, Submitted));
    assert(!tracker.ShouldReport(Shadow, NoCasters));

    Report(Bloom, Disabled);
    Report(Bloom, Disabled);
    Report(Bloom, Submitted, 1568, 713, 100, 2);
    Report(Fxaa, ShaderUnavailable);
    Report(WorldGeometry, Selected, 0, 0, 100, 4);
    assert(logLines.size() == 5); // session marker and four distinct states
    assert(logLines[0] == "[ModernRender] session start");
    assert(logLines[2].find("bloom: draw submitted; size=1568x713 scale=100 detail=2") != std::string::npos);
    assert(logLines[3].find("fxaa: shader unavailable") != std::string::npos);
    assert(logLines[4].find("world: pipeline selected") != std::string::npos);
    return 0;
}
