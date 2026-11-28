#ifndef REVC_MODERN_PEDAL_INPUT_H
#define REVC_MODERN_PEDAL_INPUT_H

// Input normalization only; physical pedal demand is not vehicle speed.
namespace ModernPedalInput {
inline float ClampUnit(float value)
{
	if (!(value > 0.0f)) return 0.0f; // includes NaN
	return value < 1.0f ? value : 1.0f;
}
inline int Pedal(float value)
{
	value = ClampUnit(value);
	// A small rest deadzone avoids creep. The remainder stays linear.
	const float deadzone = 0.03f;
	if (value <= deadzone) return 0;
	return (int)(((value - deadzone) / (1.0f - deadzone)) * 255.0f + 0.5f);
}
inline int Combine(int pedal, int keyboard, int mouse)
{
	if (keyboard > pedal) pedal = keyboard;
	if (mouse > pedal) pedal = mouse;
	return pedal;
}
}
#endif
