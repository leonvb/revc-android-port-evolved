from pathlib import Path
pad = Path('src/core/Pad.cpp').read_text()
sdl = Path('src/skel/sdl2/sdl2.cpp').read_text()
glfw = Path('src/skel/glfw/glfw.cpp').read_text()
assert 'modernPedals[i] = ModernPedals()' in pad
assert 'player->m_nPedState == PED_DRIVING' in pad
assert '!FrontEndMenuManager.GetIsMenuActive()' in pad
assert '!CReplay::IsPlayingBack()' in pad
assert 'PCTempJoyState.Cross = input.throttle' in pad
assert 'PCTempJoyState.Square = input.brake' in pad
assert 'ModernPedalInput::Combine(modernPedals[pad].throttle, PCTempKeyState.Cross, PCTempMouseState.Cross)' in pad
assert 'ModernPedalInput::Combine(modernPedals[pad].brake, PCTempKeyState.Square, PCTempMouseState.Square)' in pad
assert 'xstate.Gamepad.bLeftTrigger / 255.0f' in pad
assert 'SDL_GameControllerGetBindForAxis' in sdl
assert 'if (modernPedals) pad->SetModernVehiclePedals(leftTrigger, rightTrigger)' in sdl
assert 'capability.left && capability.right' in glfw
assert 'if (modernPedals) pad->SetModernVehiclePedals(leftTrigger, rightTrigger)' in glfw
print('Modern vehicle pedal source contract passed')
