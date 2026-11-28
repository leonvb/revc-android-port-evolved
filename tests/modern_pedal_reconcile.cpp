#include "common.h"
#include "Pad.h"
#include "ModernPedalInput.h"
#include <cassert>

int main()
{
	CControllerState keyboard, mouse, gamepad;
	keyboard.Clear(); mouse.Clear(); gamepad.Clear();
	CPad pad;
	gamepad.Cross = ModernPedalInput::Pedal(0.5f);
	gamepad.Square = ModernPedalInput::Pedal(0.25f);
	CControllerState state = pad.ReconcileTwoControllersInput(keyboard, gamepad);
	state = pad.ReconcileTwoControllersInput(mouse, state);
	// Legacy reconciliation digitalizes button fields; the Modern pedal
	// integration must restore pressures after both passes.
	assert(state.Cross == 255 && state.Square == 255);
	state.Cross = ModernPedalInput::Combine(gamepad.Cross, keyboard.Cross, mouse.Cross);
	state.Square = ModernPedalInput::Combine(gamepad.Square, keyboard.Square, mouse.Square);
	assert(state.Cross == ModernPedalInput::Pedal(0.5f));
	assert(state.Square == ModernPedalInput::Pedal(0.25f));
	keyboard.Cross = 255;
	assert(ModernPedalInput::Combine(gamepad.Cross, keyboard.Cross, mouse.Cross) == 255);
}
