#ifndef REVC_MODERN_RENDER_DIAGNOSTICS_H
#define REVC_MODERN_RENDER_DIAGNOSTICS_H

// Report each distinct state once per process, so failures are visible without
// writing to storage on every frame. A submitted draw is not GPU verification.
#ifdef ANDROID
#include "../skel/android/logger/log.h"
#endif

namespace ModernRenderDiagnostics {
enum Effect { Bloom, Fxaa, Shadow, WorldGeometry, EffectCount };
enum State {
    Disabled, CameraUnavailable, ShaderUnavailable, ReducedScale,
    InvalidCapture, AllocationFailed, TargetFailed, SceneExcluded, InvalidView,
    NoCelestialLight, FitFailed, NoCasters, Submitted, Selected, StateCount
};

struct OncePerState {
    bool seen[EffectCount][StateCount] = {};
    bool ShouldReport(Effect effect, State state) {
        if (effect < 0 || effect >= EffectCount || state < 0 || state >= StateCount)
            return false;
        if (seen[effect][state]) return false;
        seen[effect][state] = true;
        return true;
    }
};

inline void Report(Effect effect, State state, int width = 0, int height = 0,
                   int scale = 100, int detail = 0)
{
#ifdef ANDROID
    static OncePerState tracker;
    if (!tracker.ShouldReport(effect, state)) return;
    static bool sessionStarted = false;
    if (!sessionStarted) {
        Logger::Log("[ModernRender] session start");
        sessionStarted = true;
    }
    static const char *const effects[] = {"bloom", "fxaa", "shadow", "world"};
    static const char *const states[] = {
        "disabled", "camera/buffer unavailable", "shader unavailable", "render scale below 100%",
        "invalid capture bounds", "allocation failed", "target unavailable/incomplete", "interior/cutscene",
        "unsupported camera view", "no celestial light", "shadow fit failed", "no shadow casters", "draw submitted", "pipeline selected"
    };
    Logger::Log("[ModernRender] %s: %s; size=%dx%d scale=%d detail=%d",
                effects[effect], states[state], width, height, scale, detail);
#else
    (void)effect; (void)state; (void)width; (void)height; (void)scale; (void)detail;
#endif
}
}
#endif
