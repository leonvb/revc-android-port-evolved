#include "../src/core/ModernWheelInput.h"
#include <cassert>
int main()
{
	ModernWheelInput w; w.Reset();
	assert(w.Update(100, true, false, 0, 0, 8) == ModernWheelInput::NONE);
	assert(w.Update(250, false, false, 0, 0, 8) == ModernWheelInput::TAP && w.button == -1);
	w.Reset(); w.Update(1000, false, true, 0, 0, 8);
	w.Update(1179, false, true, 0, -100, 8); assert(!w.open);
	w.Update(1180, false, true, 0, -100, 8); assert(w.open && w.selected == 0);
	w.Update(1190, false, true, 100, 0, 8); assert(w.selected == 2);
	w.Update(1200, false, true, 0, 0, 8); assert(w.selected == 2);
	assert(w.Update(1210, false, false, 0, 0, 8) == ModernWheelInput::SELECT && w.selected == 2);
	w.Reset(); w.Update(UINT32_MAX - 100, true, false, 0, 0, 10);
	w.Update(100, true, false, -100, 0, 10); assert(w.open && w.selected == 8);
	w.Reset(); assert(!w.open && !w.tracking);
	w.Update(300, false, true, 0, 0, 8);
	assert(w.Update(480, false, false, 0, 0, 8) == ModernWheelInput::SELECT);
	assert(w.selected == -1);
	w.Reset(); w.Update(500, true, false, 0, 0, 8);
	w.Update(680, true, false, 20, 0, 8); assert(w.selected == -1);
	w.Update(690, true, false, 0, 100, 8); assert(w.selected == 4);
}
